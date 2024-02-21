//
// Created by caogaoshuai on 2024/1/11.
//
#include "httpdns_scheduler.h"
#include "httpdns_memory.h"
#include "http_response_parser.h"
#include "httpdns_ip.h"
#include "httpdns_sign.h"
#include "httpdns_sds.h"
#include "httpdns_log.h"
#include <pthread.h>

httpdns_scheduler_t *httpdns_scheduler_new(httpdns_config_t *config) {
    if (httpdns_config_valid(config) != HTTPDNS_SUCCESS) {
        httpdns_log_info("create httpdns scheduler failed, config is invalid");
        return NULL;
    }
    httpdns_new_object_in_heap(scheduler, httpdns_scheduler_t);
    httpdns_list_init(&scheduler->ipv4_resolve_servers);
    httpdns_list_init(&scheduler->ipv6_resolve_servers);
    scheduler->config = config;
    pthread_mutexattr_init(&scheduler->lock_attr);
    pthread_mutexattr_settype(&scheduler->lock_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&scheduler->lock, &scheduler->lock_attr);
    return scheduler;
}

httpdns_sds_t httpdns_scheduler_to_string(httpdns_scheduler_t *scheduler) {
    if (NULL == scheduler) {
        return httpdns_sds_new("httpdns_scheduler_t()");
    }
    pthread_mutex_lock(&scheduler->lock);
    httpdns_sds_t dst_str = httpdns_sds_new("httpdns_scheduler_t(ipv4_resolve_servers=");
    httpdns_sds_t list = httpdns_list_to_string(&scheduler->ipv4_resolve_servers, to_httpdns_data_to_string_func(httpdns_ip_to_string));
    httpdns_sds_cat_easily(dst_str, list);
    httpdns_sds_free(list);
    httpdns_sds_cat_easily(dst_str, ",ipv6_resolve_servers=");
    list = httpdns_list_to_string(&scheduler->ipv6_resolve_servers, to_httpdns_data_to_string_func(httpdns_ip_to_string));
    httpdns_sds_cat_easily(dst_str, list);
    httpdns_sds_free(list);
    httpdns_sds_cat_easily(dst_str, ")");
    pthread_mutex_unlock(&scheduler->lock);
    return dst_str;
}

void httpdns_scheduler_free(httpdns_scheduler_t *scheduler) {
    if (NULL == scheduler) {
        return;
    }
    httpdns_list_free(&scheduler->ipv4_resolve_servers, to_httpdns_data_free_func(httpdns_ip_free));
    httpdns_list_free(&scheduler->ipv6_resolve_servers, to_httpdns_data_free_func(httpdns_ip_free));
    pthread_mutex_destroy(&scheduler->lock);
    pthread_mutexattr_destroy(&scheduler->lock_attr);
    free(scheduler);
}


static void generate_nonce(char *nonce) {
    for (int i = 0; i < 12; i++) {
        int rand_val = (int) random() % 16;
        sprintf(&nonce[i], "%x", rand_val);
    }
    nonce[12] = '\0';
}

static void httpdns_parse_body(void *response_body, httpdns_scheduler_t *scheduler) {
    if (NULL == response_body) {
        httpdns_log_error("can't parse schedule response body, body is NULL");
        return;
    }
    httpdns_schedule_response_t *schedule_response = httpdns_response_parse_schedule(response_body);
    if (NULL != schedule_response) {
        pthread_mutex_lock(&scheduler->lock);
        httpdns_config_t *httpdns_config = scheduler->config;
        httpdns_list_for_each_entry(ipv4_resolve_server_cursor, &schedule_response->service_ip) {
            httpdns_scheduler_add_ipv4_resolve_server(scheduler, ipv4_resolve_server_cursor->data);
            if (NULL != httpdns_config) {
                httpdns_config_add_ipv4_boot_server(httpdns_config, ipv4_resolve_server_cursor->data);
            }
        }
        // 设置默认降级解析服务器
        httpdns_scheduler_add_ipv4_resolve_server(scheduler, HTTPDNS_DEFAULT_IPV4_BOOT_SERVER);
        httpdns_list_for_each_entry(ipv6_resolve_server_cursor, &schedule_response->service_ipv6) {
            httpdns_scheduler_add_ipv6_resolve_server(scheduler, ipv6_resolve_server_cursor->data);
            if (NULL != httpdns_config) {
                httpdns_config_add_ipv6_boot_server(httpdns_config, ipv6_resolve_server_cursor->data);
            }
        }
        // 设置默认降级解析服务器
        httpdns_scheduler_add_ipv6_resolve_server(scheduler, HTTPDNS_DEFAULT_IPV6_BOOT_SERVER);
        pthread_mutex_unlock(&scheduler->lock);
        httpdns_schedule_response_free(schedule_response);
        return;
    }
    httpdns_log_info("parse schedule response body failed, response body is %s", response_body);
}

int32_t httpdns_scheduler_refresh(httpdns_scheduler_t *scheduler) {
    if (NULL == scheduler || NULL == scheduler->config) {
        httpdns_log_info("refresh resolver list failed, scheduler or config is NULL");
        return HTTPDNS_PARAMETER_ERROR;
    }
    net_stack_type_t net_stack_type = httpdns_net_stack_type_get(scheduler->net_stack_detector);
    httpdns_config_t *config = scheduler->config;
    httpdns_list_head_t *boot_servers;
    if (HTTPDNS_IPV6_ONLY == net_stack_type) {
        boot_servers = &config->ipv6_boot_servers;
    } else {
        boot_servers = &config->ipv4_boot_servers;
    }
    size_t boot_server_num = httpdns_list_size(boot_servers);
    if (boot_server_num <= 0) {
        httpdns_log_info("refresh resolver list failed, boot server number is 0");
        return HTTPDNS_BOOT_SERVER_EMPTY;
    }
    const char *http_scheme = config->using_https ? HTTPDNS_HTTPS_SCHEME : HTTPDNS_HTTP_SCHEME;

    httpdns_list_node_t *first_entry = httpdns_list_first_entry(boot_servers);
    httpdns_list_node_t *cur_entry = first_entry;
    if (NULL == cur_entry) {
        httpdns_log_info("refresh resolver list failed, first entry is NULL");
        return HTTPDNS_BOOT_SERVER_EMPTY;
    }
    // 轮询启动IP， 更新IP
    do {
        httpdns_sds_t url = httpdns_sds_new(http_scheme);
        url = httpdns_sds_cat(url, cur_entry->data);
        url = httpdns_sds_cat(url, "/");
        url = httpdns_sds_cat(url, config->account_id);
        url = httpdns_sds_cat(url, "/ss?platform=linux&sdkVersion=");
        url = httpdns_sds_cat(url, config->sdk_version);
        url = httpdns_sds_cat(url, "&region=");
        url = httpdns_sds_cat(url, config->region);
        if (config->using_sign && NULL != config->secret_key) {
            char nonce[HTTPDNS_SCHEDULE_NONCE_SIZE + 1];
            generate_nonce(nonce);
            httpdns_signature_t *signature = httpdns_signature_new(nonce,
                                                                   config->secret_key,
                                                                   HTTPDNS_MAX_SCHEDULE_SIGNATURE_OFFSET_TIME,
                                                                   httpdns_time_now());
            url = httpdns_sds_cat(url, "&s=");
            url = httpdns_sds_cat(url, signature->sign);
            url = httpdns_sds_cat(url, "&t=");
            url = httpdns_sds_cat(url, signature->timestamp);
            url = httpdns_sds_cat(url, "&n=");
            url = httpdns_sds_cat(url, nonce);
            httpdns_signature_free(signature);
        }
        httpdns_http_context_t *http_context = httpdns_http_context_new(url, config->timeout_ms);
        httpdns_log_debug("exchange http request url %s", url);
        httpdns_sds_free(url);
        httpdns_http_single_exchange(http_context);
        bool success = (http_context->response_status == HTTPDNS_HTTP_STATUS_OK);
        if (success) {
            httpdns_parse_body(http_context->response_body, scheduler);
            httpdns_log_info("try server %s fetch resolve server success", cur_entry->data);
        } else {
            httpdns_sds_t http_context_str = httpdns_http_context_to_string(http_context);
            httpdns_log_info("httpdns scheduler exchange http request failed, http context is %s ", http_context_str);
            httpdns_sds_free(http_context_str);
        }
        httpdns_http_context_free(http_context);
        if (success) {
            return HTTPDNS_SUCCESS;
        }
        httpdns_log_info("try %s fetch resolve server failed", cur_entry->data);
        httpdns_list_rotate(boot_servers);
        cur_entry = httpdns_list_first_entry(boot_servers);
        httpdns_log_info("try server %s fetch resolve server", cur_entry->data);
    } while (cur_entry != first_entry);
    httpdns_log_error("all boot server fetch resolve servers failed");
    return HTTPDNS_FAILURE;
}


static int32_t update_server_rt(int32_t old_rt_val, int32_t new_rt_val) {
    if (old_rt_val == HTTPDNS_DEFAULT_IP_RT) {
        return new_rt_val;
    }
    if (new_rt_val * HTTPDNS_DELTA_WEIGHT_UPDATE_RATION > old_rt_val) {
        return old_rt_val;
    }
    return (int32_t) (new_rt_val * HTTPDNS_DELTA_WEIGHT_UPDATE_RATION + old_rt_val * (1.0 - HTTPDNS_DELTA_WEIGHT_UPDATE_RATION));
}

void httpdns_scheduler_update(httpdns_scheduler_t *scheduler, const char *server, int32_t rt) {
    if (httpdns_string_is_blank(server) || NULL == scheduler || rt <= 0) {
        httpdns_log_info("httpdns scheduler upate failed, server or scheduler or rt is invalid");
        return;
    }
    pthread_mutex_lock(&scheduler->lock);
    httpdns_ip_t *resolve_server = httpdns_list_search(&scheduler->ipv4_resolve_servers, server,
                                                       to_httpdns_data_search_func(httpdns_ip_search));
    if (NULL == resolve_server) {
        resolve_server = httpdns_list_search(&scheduler->ipv6_resolve_servers, server,
                                             to_httpdns_data_search_func(httpdns_ip_search));
    }
    if (NULL != resolve_server) {
        int32_t old_rt = resolve_server->rt;
        resolve_server->rt = update_server_rt(resolve_server->rt, rt);
        httpdns_log_debug("update resolve server %s rt success, old rt=%d, sample rt=%d, new rt=%d",
                  server,
                  old_rt,
                  rt,
                  resolve_server->rt);
    } else {
        httpdns_log_info("update resolve server failed, can't find server %s", server);
    }
    pthread_mutex_unlock(&scheduler->lock);
}

char *httpdns_scheduler_get(httpdns_scheduler_t *scheduler) {
    if (NULL == scheduler) {
        httpdns_log_info("scheduler is NULL");
        return NULL;
    }
    net_stack_type_t net_stack_type = httpdns_net_stack_type_get(scheduler->net_stack_detector);
    httpdns_list_head_t *resolve_servers;
    pthread_mutex_lock(&scheduler->lock);
    if (HTTPDNS_IPV6_ONLY == net_stack_type) {
        resolve_servers = &scheduler->ipv6_resolve_servers;
    } else {
        resolve_servers = &scheduler->ipv4_resolve_servers;
    }
    httpdns_ip_t *resolve_server = httpdns_list_min(resolve_servers, to_httpdns_data_cmp_func(httpdns_ip_cmp));
    if (NULL != resolve_server) {
        httpdns_sds_t httpdns_ip_str = httpdns_ip_to_string(resolve_server);
        httpdns_log_debug("get resolve server %s", httpdns_ip_str);
        httpdns_sds_free(httpdns_ip_str);
        httpdns_sds_t resolve_server_ip = httpdns_sds_new(resolve_server->ip);
        pthread_mutex_unlock(&scheduler->lock);
        return resolve_server_ip;
    }
    pthread_mutex_unlock(&scheduler->lock);
    httpdns_log_info("get resolve server from scheduler failed");
    return NULL;
}

void httpdns_scheduler_set_net_stack_detector(httpdns_scheduler_t *scheduler,
                                              httpdns_net_stack_detector_t *net_stack_detector) {
    if (NULL == scheduler || NULL == net_stack_detector) {
        httpdns_log_info("scheduler or net stack detector is NULL");
        return;
    }
    pthread_mutex_lock(&scheduler->lock);
    scheduler->net_stack_detector = net_stack_detector;
    pthread_mutex_unlock(&scheduler->lock);
}

void httpdns_scheduler_add_ipv4_resolve_server(httpdns_scheduler_t *scheduler,
                                               const char *resolve_server) {
    if (NULL == scheduler || NULL == resolve_server) {
        httpdns_log_info("httpdns scheduler add ipv4 resolve server failed, scheduler or resolve_server is null");
        return;
    }
    if (!httpdns_list_search(&scheduler->ipv4_resolve_servers, resolve_server, to_httpdns_data_search_func(httpdns_ip_search))) {
        httpdns_log_debug("httpdns scheduler add ipv4 resolve server %s success", resolve_server);
        httpdns_list_add(&scheduler->ipv4_resolve_servers, resolve_server, to_httpdns_data_clone_func(httpdns_ip_new));
    }
}

void httpdns_scheduler_add_ipv6_resolve_server(httpdns_scheduler_t *scheduler,
                                               const char *resolve_server) {
    if (NULL == scheduler || NULL == resolve_server) {
        httpdns_log_info("httpdns scheduler add ipv6 resolve server failed, scheduler or resolve_server is null");
        return;
    }
    if (!httpdns_list_search(&scheduler->ipv6_resolve_servers, resolve_server, to_httpdns_data_search_func(httpdns_ip_search))) {
        httpdns_log_debug("httpdns scheduler add ipv6 resolve server %s success", resolve_server);
        httpdns_list_add(&scheduler->ipv6_resolve_servers, resolve_server, to_httpdns_data_clone_func(httpdns_ip_new));
    }
}