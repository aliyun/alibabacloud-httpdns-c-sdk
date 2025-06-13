//
// Created by caogaoshuai on 2024/1/11.
//
#include <cjson/cJSON.h>
#include "hdns_http.h"
#include "hdns_log.h"
#include "hdns_sign.h"
#include "hdns_buf.h"
#include "hdns_ip.h"
#include "hdns_utils.h"

#include "hdns_scheduler.h"


static void parse_ip_array(cJSON *c_json_array, hdns_list_head_t *ips);

hdns_status_t hdns_scheduler_refresh_resolvers(hdns_scheduler_t *scheduler);

static void hdns_sched_do_parse_sched_resp(hdns_pool_t *req_pool,
                                           hdns_list_head_t *body_bufs,
                                           hdns_list_head_t *ips,
                                           hdns_list_head_t *ipsv6);

static char *generate_nonce(hdns_pool_t *pool);

static void hdns_probe_resolvers(hdns_scheduler_t *scheduler, bool ipv4);


static void parse_ip_array(cJSON *c_json_array, hdns_list_head_t *ips) {
    size_t array_size = cJSON_GetArraySize(c_json_array);
    if (array_size == 0) {
        return;
    }
    for (int i = 0; i < array_size; i++) {
        cJSON *ip_json = cJSON_GetArrayItem(c_json_array, i);
        hdns_list_add(ips, ip_json->valuestring, hdns_to_list_clone_fn_t(apr_pstrdup));
    }
}

static void hdns_sched_do_parse_sched_resp(hdns_pool_t *req_pool,
                                           hdns_list_head_t *body_bufs,
                                           hdns_list_head_t *ips,
                                           hdns_list_head_t *ipsv6) {
    char *body_str = hdns_buf_list_content(req_pool, body_bufs);
    if (hdns_str_is_blank(body_str)) {
        hdns_log_info("parse schedule response failed, body is empty");
        return;
    }
    cJSON *c_json_body = cJSON_Parse(body_str);
    if (NULL == c_json_body) {
        hdns_log_info("parse schedule response failed, body may be not json");
        return;
    }
    cJSON *ipv4_resolvers_json = cJSON_GetObjectItem(c_json_body, "service_ip");
    if (ipv4_resolvers_json != NULL) {
        parse_ip_array(ipv4_resolvers_json, ips);
        hdns_list_shuffle(ips);
    }
    cJSON *ipv6_resolvers_json = cJSON_GetObjectItem(c_json_body, "service_ipv6");
    if (NULL != ipv6_resolvers_json) {
        parse_ip_array(ipv6_resolvers_json, ipsv6);
        hdns_list_shuffle(ipsv6);
    }
    cJSON_Delete(c_json_body);
}


hdns_scheduler_t *hdns_scheduler_create(hdns_config_t *config,
                                        hdns_net_detector_t *detector,
                                        apr_thread_pool_t *thread_pool) {
    hdns_status_t status = hdns_config_valid(config);
    if (!hdns_status_is_ok(&status)) {
        hdns_log_info("create httpdns scheduler failed, config is invalid");
        return NULL;
    }
    hdns_pool_new(pool);
    hdns_scheduler_t *scheduler = hdns_palloc(pool, sizeof(hdns_scheduler_t));
    scheduler->pool = pool;
    scheduler->config = config;
    scheduler->detector = detector;
    scheduler->thread_pool = thread_pool;

    scheduler->ipv4_resolvers = hdns_list_new(NULL);
    scheduler->ipv6_resolvers = hdns_list_new(NULL);

    apr_thread_mutex_lock(config->lock);
    hdns_list_filter(scheduler->ipv4_resolvers,
                     hdns_config_get_boot_servers(config, true),
                     hdns_to_list_clone_fn_t(apr_pstrdup),
                     hdns_to_list_filter_fn_t(hdns_is_valid_ipv4));
    hdns_list_filter(scheduler->ipv6_resolvers,
                     hdns_config_get_boot_servers(config, false),
                     hdns_to_list_clone_fn_t(apr_pstrdup),
                     hdns_to_list_filter_fn_t(hdns_is_valid_ipv6));
    apr_thread_mutex_unlock(config->lock);
    scheduler->cur_ipv4_resolver_index = 0;
    scheduler->cur_ipv6_resolver_index = 0;
    apr_thread_mutex_create(&scheduler->lock, APR_THREAD_MUTEX_DEFAULT, pool);
    scheduler->state = HDNS_STATE_RUNNING;
    scheduler->next_timer_refresh_time = apr_time_now();
    scheduler->is_refreshed = false;
    return scheduler;
}

static void hdns_parse_sched_resp_body(hdns_pool_t *req_pool,
                                       hdns_list_head_t *body_bufs,
                                       hdns_scheduler_t *scheduler) {
    if (hdns_list_is_empty(body_bufs)) {
        hdns_log_error("can't parse schedule response body, body is empty");
        return;
    }
    hdns_list_head_t *ipv4_resolvers = hdns_list_new(NULL);
    hdns_list_head_t *ipv6_resolvers = hdns_list_new(NULL);

    hdns_sched_do_parse_sched_resp(req_pool, body_bufs, ipv4_resolvers, ipv6_resolvers);

    if (hdns_list_is_not_empty(ipv4_resolvers)) {
        apr_thread_mutex_lock(scheduler->lock);
        hdns_list_free(scheduler->ipv4_resolvers);
        scheduler->ipv4_resolvers = ipv4_resolvers;
        scheduler->cur_ipv4_resolver_index = 0;
        apr_thread_mutex_unlock(scheduler->lock);
        hdns_probe_resolvers(scheduler, true);
        scheduler->is_refreshed = true;
    } else {
        char *response_body = hdns_buf_list_content(req_pool, body_bufs);
        hdns_log_info("ipv4 resolver list is empty, scheduler update failed, response body is %s", response_body);
        hdns_list_free(ipv4_resolvers);
    }
    if (hdns_list_is_not_empty(ipv6_resolvers)) {
        apr_thread_mutex_lock(scheduler->lock);
        hdns_list_free(scheduler->ipv6_resolvers);
        scheduler->ipv6_resolvers = ipv6_resolvers;
        scheduler->cur_ipv6_resolver_index = 0;
        apr_thread_mutex_unlock(scheduler->lock);
        hdns_probe_resolvers(scheduler, false);
        scheduler->is_refreshed = true;
    } else {
        char *response_body = hdns_buf_list_content(req_pool, body_bufs);
        hdns_log_info("ipv6 resolver list is empty, scheduler update failed, response body is %s", response_body);
        hdns_list_free(ipv6_resolvers);
    }
}

static char *generate_nonce(hdns_pool_t *pool) {
    unsigned char random_bytes[HDNS_SCHEDULE_NONCE_SIZE / 2];
    char *hex_string = hdns_palloc(pool, HDNS_SCHEDULE_NONCE_SIZE + 1);

    apr_generate_random_bytes(random_bytes, sizeof(random_bytes));
    for (int i = 0; i < sizeof(random_bytes); i++) {
        apr_snprintf(&(hex_string[i * 2]), 3, "%02x", random_bytes[i]);
    }
    return hex_string;
}

static hdns_list_head_t *get_boot_servers(hdns_scheduler_t *scheduler, hdns_pool_t *req_pool) {
    hdns_net_type_t net_type = hdns_net_get_type(scheduler->detector);
    hdns_config_t *config = scheduler->config;
    apr_thread_mutex_lock(scheduler->config->lock);
    hdns_list_head_t *boot_servers = hdns_list_new(req_pool);
    if (HDNS_IPV6_ONLY == net_type) {
        hdns_list_dup(boot_servers, hdns_config_get_boot_servers(config, false), hdns_to_list_clone_fn_t(apr_pstrdup));
    } else {
        hdns_list_dup(boot_servers, hdns_config_get_boot_servers(config, true), hdns_to_list_clone_fn_t(apr_pstrdup));
    }
    apr_thread_mutex_unlock(scheduler->config->lock);
    return boot_servers;
}

static hdns_http_request_t *create_hdns_schd_req(hdns_scheduler_t *scheduler, hdns_pool_t *req_pool) {
    hdns_http_request_t *req = hdns_http_request_create(req_pool);
    hdns_config_t *config = scheduler->config;
    apr_thread_mutex_lock(config->lock);
    bool using_sign = config->using_sign;
    char *account_id = apr_pstrdup(req_pool, config->account_id);
    char *region = apr_pstrdup(req_pool, config->region);
    char *secret_key = apr_pstrdup(req_pool, config->secret_key);
    apr_thread_mutex_unlock(config->lock);
    // 调度服务始终使用HTTPS协议
    req->proto = HDNS_HTTPS_PREFIX;
    req->uri = apr_pstrcat(req_pool, "/", account_id, "/ss", NULL);
    apr_table_set(req->query_params, "region", region);
    apr_table_set(req->query_params, "platform", HDNS_PLATFORM);
    apr_table_set(req->query_params, "sdk_version", HDNS_VER);
    apr_table_set(req->query_params, "sid", scheduler->config->session_id);
    if (using_sign && hdns_str_is_not_blank(secret_key)) {
        char *nonce = generate_nonce(req_pool);
        hdns_sign_t *signature = hdns_gen_sched_req_sign(req_pool,
                                                         nonce,
                                                         secret_key);
        apr_table_set(req->query_params, "s", signature->sign);
        apr_table_set(req->query_params, "t", signature->timestamp);
        apr_table_set(req->query_params, "n", nonce);
    }
    return req;
}

typedef struct {
    hdns_scheduler_t *scheduler;
    hdns_pool_t *pool;
} hdns_sched_refresh_task_param_t;

static void *APR_THREAD_FUNC hdns_sched_refresh_task(apr_thread_t *thread, void *data) {
    hdns_unused_var(thread);
    hdns_sched_refresh_task_param_t *param = data;
    hdns_status_t status = hdns_scheduler_refresh_resolvers(param->scheduler);
    if (hdns_status_is_ok(&status)) {
        hdns_log_info("Asynchronous scheduler update successfully.");
    } else {
        hdns_log_error("hdns_status: %d, %s, %s", status.code, status.error_code, status.error_msg);
    }
    hdns_pool_destroy(param->pool);
    return NULL;
}

static void *APR_THREAD_FUNC hdns_sched_refresh_timer_task(apr_thread_t *thread, void *data) {
    hdns_unused_var(thread);
    hdns_unused_var(data);
    hdns_sched_refresh_task_param_t *param = data;
    hdns_scheduler_t *scheduler = param->scheduler;
    while (scheduler->state != HDNS_STATE_STOPPING) {
        if (apr_time_now() > scheduler->next_timer_refresh_time) {
            hdns_scheduler_refresh_resolvers(scheduler);
            scheduler->next_timer_refresh_time = (scheduler->is_refreshed ?
                                                  (apr_time_now() + 6 * 60 * 60 * APR_USEC_PER_SEC) :
                                                  (apr_time_now() + 5 * 60 * APR_USEC_PER_SEC));
        }
        apr_sleep(APR_USEC_PER_SEC / 2);
    }
    hdns_log_info("timer refresh task terminated.");
    hdns_pool_destroy(param->pool);
    return NULL;
}

hdns_status_t hdns_scheduler_refresh_async(hdns_scheduler_t *scheduler) {
    hdns_pool_new(pool);
    hdns_sched_refresh_task_param_t *task_param = hdns_palloc(pool, sizeof(hdns_sched_refresh_task_param_t));
    task_param->scheduler = scheduler;
    task_param->pool = pool;
    apr_status_t status = apr_thread_pool_push(scheduler->thread_pool,
                                               hdns_sched_refresh_task,
                                               task_param,
                                               0,
                                               scheduler);
    if (status != APR_SUCCESS) {
        return hdns_status_error(HDNS_SCHEDULE_FAIL, HDNS_SCHEDULE_FAIL_CODE, "Submit task failed",
                                 scheduler->config->session_id);
    }
    return hdns_status_ok(scheduler->config->session_id);
}


hdns_status_t hdns_scheduler_start_refresh_timer(hdns_scheduler_t *scheduler) {
    hdns_pool_new(pool);
    hdns_sched_refresh_task_param_t *task_param = hdns_palloc(pool, sizeof(hdns_sched_refresh_task_param_t));
    task_param->scheduler = scheduler;
    task_param->pool = pool;
    apr_status_t status = apr_thread_pool_push(scheduler->thread_pool,
                                               hdns_sched_refresh_timer_task,
                                               task_param,
                                               0,
                                               scheduler);

    if (status != APR_SUCCESS) {
        return hdns_status_error(HDNS_SCHEDULE_FAIL, HDNS_SCHEDULE_FAIL_CODE, "Submit task failed",
                                 scheduler->config->session_id);
    }
    return hdns_status_ok(scheduler->config->session_id);
}



hdns_status_t hdns_scheduler_refresh_resolvers(hdns_scheduler_t *scheduler) {
    hdns_status_t status;
    hdns_pool_new(req_pool);

    hdns_list_head_t *boot_servers = get_boot_servers(scheduler, req_pool);

    size_t boot_server_size = hdns_list_size(boot_servers);
    if (boot_server_size <= 0) {
        hdns_pool_destroy(req_pool);
        return hdns_status_error(HDNS_SCHEDULE_FAIL, HDNS_SCHEDULE_FAIL_CODE, "boot server list is empty",
                                 scheduler->config->session_id);
    }
    hdns_http_request_t *req = create_hdns_schd_req(scheduler, req_pool);


    for (int i = 0; i < boot_server_size; i++) {
        char *boot_server = hdns_list_get(boot_servers, i);
        if (hdns_str_is_blank(boot_server)) {
            status = hdns_status_error(HDNS_SCHEDULE_FAIL, HDNS_SCHEDULE_FAIL_CODE, "boot server is null",
                                       scheduler->config->session_id);
            continue;
        }
        hdns_log_info("try server %s fetch resolve server", boot_server);
        req->host = apr_pstrdup(req_pool, boot_server);

        hdns_http_controller_t *ctl = hdns_http_controller_create(req_pool);
        apr_thread_mutex_lock(scheduler->config->lock);
        ctl->timeout = scheduler->config->timeout;
        apr_thread_mutex_unlock(scheduler->config->lock);

        hdns_http_response_t *http_resp = hdns_http_response_create(req_pool);

        hdns_http_send_request(ctl, req, http_resp);

        // 建连失败
        if (http_resp->status <= 0) {
            status = hdns_status_error(HDNS_RESOLVE_FAIL,
                                       HDNS_RESOLVE_FAIL_CODE,
                                       http_resp->extra_info->reason,
                                       scheduler->config->session_id);
            continue;
        }
        if (http_resp->status == HDNS_HTTP_STATUS_OK) {
            hdns_parse_sched_resp_body(req_pool, http_resp->body, scheduler);
            hdns_log_info("try server %s fetch resolve server success", boot_server);
            status = hdns_status_ok(scheduler->config->session_id);
            break;
        } else {
            char *resp_body = hdns_buf_list_content(req_pool, http_resp->body);
            hdns_log_info("httpdns scheduler exchange http request failed, http body is %s ", resp_body);
            status = hdns_status_error(HDNS_SCHEDULE_FAIL, HDNS_SCHEDULE_FAIL_CODE, resp_body,
                                       scheduler->config->session_id);
        }
    }
    hdns_pool_destroy(req_pool);
    return status;
}

int hdns_scheduler_get(hdns_scheduler_t *scheduler, char *resolver) {
    if (NULL == scheduler) {
        hdns_log_info("scheduler is NULL");
        return HDNS_ERROR;
    }
    hdns_net_type_t net_stack_type = hdns_net_get_type(scheduler->detector);
    hdns_list_head_t *resolve_servers;
    int32_t resolver_index;
    apr_thread_mutex_lock(scheduler->lock);
    // 确保一定返回一个resolver
    if (HDNS_IPV6_ONLY == net_stack_type) {
        resolve_servers = scheduler->ipv6_resolvers;
        // 可能是新的resolver还没有从服务端拉回，此时重新轮询现有的服务节点
        if (scheduler->cur_ipv6_resolver_index >= hdns_list_size(resolve_servers)) {
            scheduler->cur_ipv6_resolver_index = 0;
        }
        resolver_index = scheduler->cur_ipv6_resolver_index;
    } else {
        resolve_servers = scheduler->ipv4_resolvers;
        if (scheduler->cur_ipv4_resolver_index >= hdns_list_size(resolve_servers)) {
            scheduler->cur_ipv4_resolver_index = 0;
        }
        resolver_index = scheduler->cur_ipv4_resolver_index;
    }

    char *resolve_server = hdns_list_get(resolve_servers, resolver_index);
    if (NULL != resolve_server && hdns_str_is_not_blank(resolve_server)) {
        sprintf(resolver, "%s", resolve_server);
        apr_thread_mutex_unlock(scheduler->lock);
        return HDNS_OK;
    }

    apr_thread_mutex_unlock(scheduler->lock);
    hdns_log_info("get resolve server from scheduler failed");
    return HDNS_ERROR;
}

void hdns_scheduler_failover(hdns_scheduler_t *scheduler, const char *server) {
    if (hdns_str_is_blank(server) || NULL == scheduler) {
        hdns_log_info("httpdns scheduler failover failed, server or scheduler is invalid");
        return;
    }
    apr_thread_mutex_lock(scheduler->lock);
    if (hdns_is_valid_ipv4(server)) {
        scheduler->cur_ipv4_resolver_index++;
    } else if (hdns_is_valid_ipv6(server)) {
        scheduler->cur_ipv6_resolver_index++;
    } else {
        scheduler->cur_ipv4_resolver_index++;
        scheduler->cur_ipv6_resolver_index++;
    }
    if (scheduler->cur_ipv6_resolver_index >= hdns_list_size(scheduler->ipv6_resolvers)) {
        hdns_scheduler_refresh_async(scheduler);
    } else if (scheduler->cur_ipv4_resolver_index >= hdns_list_size(scheduler->ipv4_resolvers)) {
        hdns_scheduler_refresh_async(scheduler);
    }
    apr_thread_mutex_unlock(scheduler->lock);
}


int hdns_scheduler_cleanup(hdns_scheduler_t *scheduler) {
    if (scheduler != NULL) {
        scheduler->state = HDNS_STATE_STOPPING;
        apr_thread_pool_tasks_cancel(scheduler->thread_pool, scheduler);
        apr_thread_mutex_destroy(scheduler->lock);
        hdns_list_free(scheduler->ipv4_resolvers);
        hdns_list_free(scheduler->ipv6_resolvers);
        hdns_pool_destroy(scheduler->pool);
    }
    return HDNS_OK;
}

typedef struct {
    hdns_pool_t *pool;
    hdns_scheduler_t *scheduler;
    bool ipv4;
} hdns_net_speed_resolver_cb_fn_param_t;

void hdns_net_speed_resolver_cb_fn(hdns_list_head_t *sorted_ips, void *user_params) {
    hdns_net_speed_resolver_cb_fn_param_t *param = user_params;
    if (hdns_list_is_empty(sorted_ips)) {
        hdns_pool_destroy(param->pool);
        return;
    }
    hdns_scheduler_t *scheduler = param->scheduler;
    hdns_list_head_t *resolvers = hdns_list_new(NULL);
    hdns_list_for_each_entry_safe(sorted_cursor, sorted_ips) {
        hdns_ip_t *sorted_ip = sorted_cursor->data;
        hdns_list_add(resolvers, sorted_ip->ip, hdns_to_list_clone_fn_t(apr_pstrdup));
    }
    apr_thread_mutex_lock(param->scheduler->lock);
    if (param->ipv4) {
        hdns_list_free(scheduler->ipv4_resolvers);
        scheduler->ipv4_resolvers = resolvers;
        scheduler->cur_ipv4_resolver_index = 0;
    } else {
        hdns_list_free(scheduler->ipv6_resolvers);
        scheduler->ipv6_resolvers = resolvers;
        scheduler->cur_ipv6_resolver_index = 0;
    }
    apr_thread_mutex_unlock(scheduler->lock);
    hdns_pool_destroy(param->pool);
}

static void hdns_probe_resolvers(hdns_scheduler_t *scheduler, bool ipv4) {
    hdns_pool_new(pool);
    hdns_list_head_t *resolvers = hdns_list_new(pool);
    apr_thread_mutex_lock(scheduler->lock);
    hdns_list_dup(resolvers,
                  ipv4 ? scheduler->ipv4_resolvers : scheduler->ipv6_resolvers,
                  hdns_to_list_clone_fn_t(apr_pstrdup));
    apr_thread_mutex_unlock(scheduler->lock);

    hdns_net_speed_resolver_cb_fn_param_t *param = hdns_palloc(pool, sizeof(hdns_net_speed_resolver_cb_fn_param_t));
    param->pool = pool;
    param->scheduler = scheduler;
    param->ipv4 = ipv4;
    hdns_net_add_speed_detect_task(scheduler->detector,
                                   hdns_net_speed_resolver_cb_fn,
                                   param,
                                   resolvers,
                                   80,
                                   scheduler,
                                   &(scheduler->state)
                                   );

}
