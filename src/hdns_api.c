//
// Created by cagaoshuai on 2024/4/9.
//
#include "hdns_api.h"
#include "hdns_log.h"
#include "hdns_status.h"
#include "hdns_client.h"
#include "hdns_session.h"
#include "apr_thread_pool.h"

#define HDNS_THREAD_POOL_CORE_SIZE  4
#define HDNS_THREAD_POOL_MAX_SIZE   16
#define HDNS_THREAD_IDLE_TIME       30
#define HDNS_THREAD_MAX_TASK_COUNT  200

#define CHECK_HDNS_TASK_COUNT() \
    do { \
        if (apr_thread_pool_tasks_count(g_hdns_api_thread_pool) > HDNS_THREAD_MAX_TASK_COUNT) { \
            return hdns_status_error(HDNS_RESOLVE_FAIL, HDNS_RESOLVE_FAIL_CODE, "too many async tasks", NULL); \
        } \
    } while(0)


static hdns_pool_t *g_hdns_api_pool = NULL;

static apr_thread_pool_t *g_hdns_api_thread_pool = NULL;

static volatile int32_t g_hdns_api_initialized = 0;

static hdns_net_detector_t *g_hdns_net_detector = NULL;


static void empty_hdns_resv_done_callback(hdns_status_t *status, hdns_list_head_t *results, void *param) {
    hdns_unused_var(status);
    hdns_unused_var(results);
    hdns_unused_var(param);
    if (NULL == status) {
        hdns_log_error("Pre-resolution failed.");
    }
    if (!hdns_status_is_ok(status)) {
        hdns_log_error("Pre-resolution failed: code:%d, error_code:%s, error_msg:%s, session_id:%s",
                       status->code,
                       status->error_code,
                       status->error_msg,
                       status->session_id);
    }
}


int hdns_sdk_init() {
    if (g_hdns_api_initialized++) {
        return HDNS_OK;
    }
    int s;
    char buf[256];
    if ((apr_initialize()) != APR_SUCCESS) {
        hdns_log_fatal("APR initialization failed.");
        return HDNS_ERROR;
    }
    if ((s = hdns_pool_create(&g_hdns_api_pool, NULL)) != APR_SUCCESS) {
        hdns_log_fatal("hdns_pool_create failure, code:%d %s.\n", s, apr_strerror(s, buf, sizeof(buf)));
        return HDNS_ERROR;
    }
    hdns_log_create(g_hdns_api_pool);
    //  最多个允许保留20个内存块，内存块要尽快归还操作系统
    apr_allocator_max_free_set(apr_pool_allocator_get(g_hdns_api_pool), 20);

    if ((s = apr_thread_pool_create(&g_hdns_api_thread_pool,
                                    HDNS_THREAD_POOL_CORE_SIZE,
                                    HDNS_THREAD_POOL_MAX_SIZE,
                                    g_hdns_api_pool)) != APR_SUCCESS) {
        hdns_log_fatal("thread_pool_create failure, code:%d %s.\n", s, apr_strerror(s, buf, sizeof(buf)));
        return HDNS_ERROR;
    }
    apr_thread_pool_idle_wait_set(g_hdns_api_thread_pool, apr_time_from_sec(HDNS_THREAD_IDLE_TIME));

    g_hdns_net_detector = hdns_net_detector_create(g_hdns_api_thread_pool);

    srand((unsigned) time(NULL));

    return hdns_session_pool_init(g_hdns_api_pool, 0);
}


hdns_client_t *hdns_client_create(const char *account_id, const char *secret_key) {
    hdns_config_t *config = hdns_config_create();
    config->account_id = apr_pstrdup(config->pool, account_id);
    if (hdns_str_is_not_blank(secret_key)) {
        config->secret_key = apr_pstrdup(config->pool, secret_key);
        config->using_sign = true;
    }

    hdns_pool_new(pool);
    hdns_client_t *client = hdns_palloc(pool, sizeof(hdns_client_t));
    client->pool = pool;
    client->config = config;
    client->net_detector = g_hdns_net_detector;
    client->scheduler = hdns_scheduler_create(config, g_hdns_net_detector, g_hdns_api_thread_pool);
    client->cache = hdns_cache_table_create();
    client->state = HDNS_STATE_INIT;
    return client;
}

void hdns_client_add_pre_resolve_host(hdns_client_t *client, const char *host) {
    apr_thread_mutex_lock(client->config->lock);
    hdns_list_add(client->config->pre_resolve_hosts, host, hdns_to_list_clone_fn_t(apr_pstrdup));
    apr_thread_mutex_unlock(client->config->lock);
}


hdns_status_t hdns_client_start(hdns_client_t *client) {
    if (NULL == client) {
        return hdns_status_error(HDNS_INVALID_ARGUMENT, HDNS_INVALID_ARGUMENT_CODE, "The client is null.", NULL);
    }
    client->state = HDNS_STATE_START;
    // 定时刷新解析服务IP列表
    hdns_status_t status = hdns_scheduler_start_refresh_timer(client->scheduler);
    if (!hdns_status_is_ok(&status)) {
        return status;
    }
    // 异步预解析
    status = hdns_get_results_for_hosts_async_with_cache(client,
                                                         client->config->pre_resolve_hosts,
                                                         HDNS_QUERY_AUTO,
                                                         NULL,
                                                         empty_hdns_resv_done_callback,
                                                         NULL);
    if (!hdns_status_is_ok(&status)) {
        return status;
    }
    client->state = HDNS_STATE_RUNNING;
    return hdns_status_ok(client->config->session_id);
}


void hdns_client_set_timeout(hdns_client_t *client, int32_t timeout) {
    if (timeout > 0) {
        apr_thread_mutex_lock(client->config->lock);
        client->config->timeout = timeout;
        apr_thread_mutex_unlock(client->config->lock);
    }
}

void hdns_client_set_using_cache(hdns_client_t *client, bool using_cache) {
    apr_thread_mutex_lock(client->config->lock);
    client->config->using_cache = using_cache;
    apr_thread_mutex_unlock(client->config->lock);
}

void hdns_client_set_using_https(hdns_client_t *client, bool using_https) {
    apr_thread_mutex_lock(client->config->lock);
    client->config->using_https = using_https;
    apr_thread_mutex_unlock(client->config->lock);
}

void hdns_client_set_using_sign(hdns_client_t *client, bool using_sign) {
    apr_thread_mutex_lock(client->config->lock);
    client->config->using_sign = using_sign;
    apr_thread_mutex_unlock(client->config->lock);
}

void hdns_client_set_retry_times(hdns_client_t *client, int32_t retry_times) {
    if (retry_times < 0) {
        return;
    }
    apr_thread_mutex_lock(client->config->lock);
    client->config->retry_times = retry_times;
    apr_thread_mutex_unlock(client->config->lock);
}

void hdns_client_set_region(hdns_client_t *client, const char *region) {
    apr_thread_mutex_lock(client->config->lock);
    if (strcmp(region, client->config->region)) {
        client->config->region = apr_pstrdup(client->config->pool, region);
        if (client->state == HDNS_STATE_RUNNING) {
            hdns_cache_table_clean(client->cache);
            hdns_scheduler_refresh_async(client->scheduler);
        }
    }
    apr_thread_mutex_unlock(client->config->lock);
}

void hdns_client_set_schedule_center_region(hdns_client_t *client, const char *region) {
    apr_thread_mutex_lock(client->config->lock);
    bool changed = strcmp(region, client->config->boot_server_region);
    if (changed) {
        client->config->boot_server_region = apr_pstrdup(client->config->pool, region);
    }
    apr_thread_mutex_unlock(client->config->lock);

    apr_thread_mutex_lock(client->scheduler->lock);
    if (changed) {
        hdns_scheduler_t *scheduler = client->scheduler;
        hdns_list_free(scheduler->ipv4_resolvers);
        hdns_list_free(scheduler->ipv6_resolvers);
        scheduler->cur_ipv4_resolver_index = 0;
        scheduler->cur_ipv6_resolver_index = 0;
        scheduler->ipv4_resolvers = hdns_list_new(NULL);
        scheduler->ipv6_resolvers = hdns_list_new(NULL);

        hdns_list_dup(scheduler->ipv4_resolvers,
                      hdns_config_get_boot_servers(client->config, true), hdns_to_list_clone_fn_t(apr_pstrdup));
        hdns_list_dup(scheduler->ipv6_resolvers,
                      hdns_config_get_boot_servers(client->config, false), hdns_to_list_clone_fn_t(apr_pstrdup));

    }
    apr_thread_mutex_unlock(client->scheduler->lock);
}

void hdns_client_enable_update_cache_after_net_change(hdns_client_t *client, bool enable) {
    if (enable) {
        hdns_net_add_chg_cb_task(client->net_detector,
                                 HDNS_NET_CB_UPDATE_CACHE,
                                 (hdns_net_chg_cb_fn_t) hdns_update_cache_on_net_change,
                                 client,
                                 client->cache);
    } else {
        hdns_net_cancel_chg_cb_task(client->net_detector,
                                    client->cache);
    }
}

void hdns_client_enable_expired_ip(hdns_client_t *client, bool enable) {
    apr_thread_mutex_lock(client->config->lock);
    client->config->enable_expired_ip = enable;
    apr_thread_mutex_unlock(client->config->lock);
}

void hdns_client_enable_failover_localdns(hdns_client_t *client, bool enable) {
    apr_thread_mutex_lock(client->config->lock);
    client->config->enable_failover_localdns = enable;
    apr_thread_mutex_unlock(client->config->lock);
}

void hdns_config_add_pre_resolve_host(hdns_client_t *client, const char *host) {
    apr_thread_mutex_lock(client->config->lock);
    hdns_list_add(client->config->pre_resolve_hosts, host, hdns_to_list_clone_fn_t(apr_pstrdup));
    apr_thread_mutex_unlock(client->config->lock);
}


void hdns_client_add_ip_probe_item(hdns_client_t *client, const char *host, const int port) {
    apr_thread_mutex_lock(client->config->lock);
    hdns_config_t *config = client->config;
    int *port_pr = hdns_palloc(config->pool, sizeof(int));
    *port_pr = port;
    apr_hash_set(config->ip_probe_items, apr_pstrdup(config->pool, host), APR_HASH_KEY_STRING, port_pr);
    apr_thread_mutex_unlock(client->config->lock);
}

void hdns_client_add_custom_ttl_item(hdns_client_t *client, const char *host, const int ttl) {
    apr_thread_mutex_lock(client->config->lock);
    hdns_config_t *config = client->config;
    int *ttl_pr = hdns_palloc(config->pool, sizeof(int));
    *ttl_pr = ttl;
    apr_hash_set(config->custom_ttl_items, apr_pstrdup(config->pool, host), APR_HASH_KEY_STRING, ttl_pr);
    apr_thread_mutex_unlock(client->config->lock);
}

int hdns_client_get_session_id(hdns_client_t *client, char *session_id) {
    if (NULL == client || NULL == client->config || NULL == client->config->session_id) {
        return HDNS_ERROR;
    }
    strcpy(session_id, client->config->session_id);
    return HDNS_OK;
}


hdns_resv_req_t *hdns_resv_req_create(hdns_client_t *client) {
    return hdns_resv_req_new(NULL, client->config);
}

hdns_status_t hdns_resv_req_set_client_ip(hdns_resv_req_t *req, const char *client_ip) {
    if (NULL == req || hdns_str_is_blank(client_ip)) {
        return hdns_status_error(HDNS_INVALID_ARGUMENT,
                                 HDNS_INVALID_ARGUMENT_CODE,
                                 "req is null or client_ip is blank",
                                 req->session_id);

    }
    apr_thread_mutex_lock(req->lock);
    req->client_ip = apr_pstrdup(req->pool, client_ip);
    apr_thread_mutex_unlock(req->lock);
    return hdns_status_ok(req->session_id);
}

hdns_status_t hdns_resv_req_set_host(hdns_resv_req_t *req, const char *host) {
    if (NULL == req || hdns_str_is_blank(host)) {
        return hdns_status_error(HDNS_INVALID_ARGUMENT,
                                 HDNS_INVALID_ARGUMENT_CODE,
                                 "req is null or host is blank",
                                 req->session_id);

    }
    apr_thread_mutex_lock(req->lock);
    req->host = apr_pstrdup(req->pool, host);
    apr_thread_mutex_unlock(req->lock);
    return hdns_status_ok(req->session_id);
}

hdns_status_t hdns_resv_req_append_sdns_param(hdns_resv_req_t *req, const char *key, const char *value) {
    if (NULL == req || hdns_str_is_blank(key) || hdns_str_is_blank(value)) {
        return hdns_status_error(HDNS_INVALID_ARGUMENT,
                                 HDNS_INVALID_ARGUMENT_CODE,
                                 "req is null or host is blank or value is blank",
                                 req->session_id);

    }
    apr_thread_mutex_lock(req->lock);
    apr_table_set(req->sdns_params, key, value);
    apr_thread_mutex_unlock(req->lock);
    return hdns_status_ok(req->session_id);
}

hdns_status_t hdns_resv_req_set_query_type(hdns_resv_req_t *req, hdns_query_type_t query_type) {
    if (NULL == req || query_type > HDNS_QUERY_BOTH) {
        return hdns_status_error(HDNS_INVALID_ARGUMENT,
                                 HDNS_INVALID_ARGUMENT_CODE,
                                 "req is null or query_type is invalid",
                                 req->session_id);

    }
    apr_thread_mutex_lock(req->lock);
    req->query_type = query_type;
    apr_thread_mutex_unlock(req->lock);
    return hdns_status_ok(req->session_id);
}

hdns_status_t hdns_resv_req_set_cache_key(hdns_resv_req_t *req, const char *cache_key) {
    if (NULL == req || hdns_str_is_blank(cache_key)) {
        return hdns_status_error(HDNS_INVALID_ARGUMENT,
                                 HDNS_INVALID_ARGUMENT_CODE,
                                 "req is null or cache_key is blank",
                                 req->session_id);

    }
    apr_thread_mutex_lock(req->lock);
    req->cache_key = apr_pstrdup(req->pool, cache_key);
    apr_thread_mutex_unlock(req->lock);
    return hdns_status_ok(req->session_id);
}

void hdns_resv_req_cleanup(hdns_resv_req_t *req) {
    hdns_resv_req_free(req);
}

void hdns_list_cleanup(hdns_list_head_t *list) {
    if (list != NULL && list->pool != NULL) {
        hdns_pool_destroy(list->pool);
    }
}

hdns_list_head_t *hdns_list_create() {
    return hdns_list_new(NULL);
}

hdns_status_t hdns_list_add_str(hdns_list_head_t *list, const char *str) {
    if (list != NULL && list->pool != NULL && hdns_str_is_not_blank(str)) {
        hdns_list_add(list, str, hdns_to_list_clone_fn_t(apr_pstrdup));
        return hdns_status_ok(NULL);
    }
    return hdns_status_error(HDNS_INVALID_ARGUMENT,
                             HDNS_INVALID_ARGUMENT_CODE,
                             "list is invalid or str is blank",
                             NULL);
}

hdns_status_t hdns_get_result_for_host_sync_with_custom_request(hdns_client_t *client,
                                                                const hdns_resv_req_t *req,
                                                                hdns_list_head_t **results) {
    if (NULL == results) {
        return hdns_status_error(HDNS_INVALID_ARGUMENT,
                                 HDNS_INVALID_ARGUMENT_CODE,
                                 "The results_pt is null.",
                                 client->config->session_id);
    }
    hdns_status_t status = hdns_resv_req_valid(req);
    if (!hdns_status_is_ok(&status)) {
        return status;
    }

    hdns_list_head_t *tmp_results = hdns_list_create();
    hdns_resv_req_t *tmp_req = hdns_resv_req_clone(NULL, req);
    status = hdns_do_single_resolve_with_req(client, tmp_req, tmp_results);
    hdns_resv_req_free(tmp_req);
    if (!hdns_status_is_ok(&status)) {
        hdns_list_free(tmp_results);
        (*results) = NULL;
    } else {
        (*results) = tmp_results;
    }
    return status;
}


static hdns_status_t hdns_get_result_for_host_sync(hdns_client_t *client,
                                                   const char *host,
                                                   hdns_query_type_t query_type,
                                                   const char *client_ip,
                                                   bool using_cache,
                                                   hdns_list_head_t **results) {
    if (NULL == results) {
        return hdns_status_error(HDNS_INVALID_ARGUMENT,
                                 HDNS_INVALID_ARGUMENT_CODE,
                                 "The results is null.",
                                 client->config->session_id);
    }
    hdns_pool_new(pool);
    char *tmp_host = apr_pstrdup(pool, host);
    hdns_list_head_t *tmp_results = hdns_list_create();
    hdns_status_t status = hdns_do_single_resolve(client, tmp_host, query_type, using_cache, client_ip, tmp_results);
    hdns_pool_destroy(pool);
    if (!hdns_status_is_ok(&status)) {
        hdns_list_free(tmp_results);
        (*results) = NULL;
    } else {
        (*results) = tmp_results;
    }
    return status;
}


hdns_status_t hdns_get_result_for_host_sync_with_cache(hdns_client_t *client,
                                                       const char *host,
                                                       hdns_query_type_t query_type,
                                                       const char *client_ip,
                                                       hdns_list_head_t **results) {
    return hdns_get_result_for_host_sync(client, host, query_type, client_ip, true, results);
}

hdns_status_t hdns_get_result_for_host_sync_without_cache(hdns_client_t *client,
                                                          const char *host,
                                                          hdns_query_type_t query_type,
                                                          const char *client_ip,
                                                          hdns_list_head_t **results) {
    return hdns_get_result_for_host_sync(client, host, query_type, client_ip, false, results);
}

static hdns_status_t hdns_get_results_for_hosts_sync(hdns_client_t *client,
                                                     const hdns_list_head_t *hosts,
                                                     hdns_query_type_t query_type,
                                                     const char *client_ip,
                                                     bool using_cache,
                                                     hdns_list_head_t **results) {
    if (NULL == results) {
        return hdns_status_error(HDNS_INVALID_ARGUMENT,
                                 HDNS_INVALID_ARGUMENT_CODE,
                                 "The results_pt is null.",
                                 client->config->session_id);
    }
    if (hdns_list_is_empty(hosts)) {
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "hosts is empty",
                                 client->config->session_id);
    }
    if (query_type > HDNS_QUERY_BOTH) {
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "query_type is invalid",
                                 client->config->session_id);
    }
    hdns_list_head_t *tmp_hosts = hdns_list_create();
    hdns_list_dup(tmp_hosts, hosts, hdns_to_list_clone_fn_t(apr_pstrdup));
    hdns_list_head_t *tmp_results = hdns_list_create();
    hdns_status_t status = hdns_do_batch_resolve(client, tmp_hosts, query_type, using_cache, client_ip, tmp_results);
    hdns_list_free(tmp_hosts);
    if (!hdns_status_is_ok(&status)) {
        hdns_list_free(tmp_results);
        (*results) = NULL;
    } else {
        (*results) = tmp_results;
    }
    return status;
}

hdns_status_t hdns_get_results_for_hosts_sync_with_cache(hdns_client_t *client,
                                                         const hdns_list_head_t *hosts,
                                                         hdns_query_type_t query_type,
                                                         const char *client_ip,
                                                         hdns_list_head_t **results) {
    return hdns_get_results_for_hosts_sync(client, hosts, query_type, client_ip, true, results);
}

hdns_status_t hdns_get_results_for_hosts_sync_without_cache(hdns_client_t *client,
                                                            const hdns_list_head_t *hosts,
                                                            hdns_query_type_t query_type,
                                                            const char *client_ip,
                                                            hdns_list_head_t **results) {
    return hdns_get_results_for_hosts_sync(client, hosts, query_type, client_ip, false, results);
}


typedef struct {
    hdns_pool_t *pool;
    hdns_client_t *client;
    hdns_resv_req_t *resv_req;
    hdns_resv_done_callback_pt cb;
    void *cb_param;
} hdns_single_resv_with_custom_req_param_t;

static void *APR_THREAD_FUNC hdns_single_resv_with_custom_req_task(apr_thread_t *thread, void *data) {
    hdns_unused_var(thread);
    hdns_single_resv_with_custom_req_param_t *param = data;
    hdns_list_head_t *results = NULL;
    hdns_status_t status = hdns_get_result_for_host_sync_with_custom_request(param->client, param->resv_req, &results);
    param->cb(&status, results, param->cb_param);
    hdns_list_free(results);
    hdns_pool_destroy(param->pool);
    return NULL;
}

hdns_status_t hdns_get_result_for_host_async_with_custom_request(hdns_client_t *client,
                                                                 const hdns_resv_req_t *resv_req,
                                                                 hdns_resv_done_callback_pt cb,
                                                                 void *cb_param) {
    CHECK_HDNS_TASK_COUNT();
    if (NULL == cb) {
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "callback is null.",
                                 client->config->session_id);
    }

    hdns_pool_new(pool);
    hdns_single_resv_with_custom_req_param_t *task_param = hdns_palloc(pool,
                                                                       sizeof(hdns_single_resv_with_custom_req_param_t));

    task_param->pool = pool;
    task_param->client = client;
    task_param->resv_req = hdns_resv_req_clone(pool, resv_req);
    task_param->cb_param = cb_param;
    task_param->cb = cb;

    apr_status_t status = apr_thread_pool_push(g_hdns_api_thread_pool,
                                               hdns_single_resv_with_custom_req_task,
                                               task_param,
                                               0,
                                               client);
    if (status != APR_SUCCESS) {
        hdns_pool_destroy(pool);
        return hdns_status_error(HDNS_RESOLVE_FAIL, HDNS_RESOLVE_FAIL_CODE, "Submit task failed",
                                 client->config->session_id);
    }
    return hdns_status_ok(client->config->session_id);
}

typedef struct {
    hdns_pool_t *pool;
    hdns_client_t *client;
    const char *host;
    hdns_query_type_t query_type;
    const char *client_ip;
    bool using_cache;
    hdns_resv_done_callback_pt cb;
    void *cb_param;
} hdns_single_resv_task_param_t;

static void *APR_THREAD_FUNC hdns_single_resv_task(apr_thread_t *thread, void *data) {
    hdns_unused_var(thread);
    hdns_single_resv_task_param_t *param = data;
    hdns_list_head_t *results = NULL;
    hdns_status_t status = hdns_get_result_for_host_sync(param->client,
                                                         param->host,
                                                         param->query_type,
                                                         param->client_ip,
                                                         param->using_cache,
                                                         &results);
    param->cb(&status, results, param->cb_param);
    hdns_list_free(results);
    hdns_pool_destroy(param->pool);
    return NULL;
}

hdns_status_t hdns_get_result_for_host_async_with_cache(hdns_client_t *client,
                                                        const char *host,
                                                        hdns_query_type_t query_type,
                                                        const char *client_ip,
                                                        hdns_resv_done_callback_pt cb,
                                                        void *cb_param) {
    CHECK_HDNS_TASK_COUNT();
    if (NULL == cb) {
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "callback is null.",
                                 client->config->session_id);
    }
    hdns_pool_new(pool);

    hdns_single_resv_task_param_t *task_param = hdns_palloc(pool, sizeof(hdns_single_resv_task_param_t));

    task_param->pool = pool;
    task_param->client = client;
    task_param->host = apr_pstrdup(pool, host);
    task_param->query_type = query_type;
    task_param->client_ip = client_ip;
    task_param->using_cache = true;
    task_param->cb = cb;
    task_param->cb_param = cb_param;

    apr_status_t status = apr_thread_pool_push(g_hdns_api_thread_pool, hdns_single_resv_task, task_param, 0, client);
    if (status != APR_SUCCESS) {
        hdns_pool_destroy(pool);
        return hdns_status_error(HDNS_RESOLVE_FAIL, HDNS_RESOLVE_FAIL_CODE, "Submit task failed",
                                 client->config->session_id);
    }
    return hdns_status_ok(client->config->session_id);
}

hdns_status_t hdns_get_result_for_host_async_without_cache(hdns_client_t *client,
                                                           const char *host,
                                                           hdns_query_type_t query_type,
                                                           const char *client_ip,
                                                           hdns_resv_done_callback_pt cb,
                                                           void *cb_param) {
    CHECK_HDNS_TASK_COUNT();
    if (NULL == cb) {
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "callback is null.",
                                 client->config->session_id);
    }
    hdns_pool_new(pool);
    hdns_single_resv_task_param_t *task_param = hdns_palloc(pool, sizeof(hdns_single_resv_task_param_t));
    task_param->pool = pool;
    task_param->client = client;
    task_param->host = apr_pstrdup(pool, host);
    task_param->query_type = query_type;
    task_param->client_ip = client_ip;
    task_param->using_cache = false;
    task_param->cb = cb;
    task_param->cb_param = cb_param;

    apr_status_t status = apr_thread_pool_push(g_hdns_api_thread_pool, hdns_single_resv_task, task_param, 0, client);
    if (status != APR_SUCCESS) {
        hdns_pool_destroy(pool);
        return hdns_status_error(HDNS_RESOLVE_FAIL, HDNS_RESOLVE_FAIL_CODE, "Submit task failed",
                                 client->config->session_id);
    }
    return hdns_status_ok(client->config->session_id);
}


typedef struct {
    hdns_pool_t *pool;
    hdns_client_t *client;
    hdns_list_head_t *hosts;
    hdns_query_type_t query_type;
    const char *client_ip;
    bool using_cache;
    hdns_resv_done_callback_pt cb;
    void *cb_param;
} hdns_batch_resv_task_param_t;


static void *APR_THREAD_FUNC hdns_batch_resv_task(apr_thread_t *thread, void *data) {
    hdns_unused_var(thread);
    hdns_batch_resv_task_param_t *param = data;
    hdns_list_head_t *results = NULL;
    hdns_status_t status = hdns_get_results_for_hosts_sync(param->client,
                                                           param->hosts,
                                                           param->query_type,
                                                           param->client_ip,
                                                           param->using_cache,
                                                           &results);
    param->cb(&status, results, param->cb_param);
    hdns_list_free(results);
    hdns_pool_destroy(param->pool);
    return NULL;
}

hdns_status_t hdns_get_results_for_hosts_async_with_cache(hdns_client_t *client,
                                                          const hdns_list_head_t *hosts,
                                                          hdns_query_type_t query_type,
                                                          const char *client_ip,
                                                          hdns_resv_done_callback_pt cb,
                                                          void *cb_param) {
    CHECK_HDNS_TASK_COUNT();
    if (NULL == cb) {
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "callback is null.",
                                 client->config->session_id);
    }
    if (hdns_list_is_empty(hosts)) {
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "hosts is empty.",
                                 client->config->session_id);
    }
    hdns_pool_new(pool);
    hdns_batch_resv_task_param_t *task_param = hdns_palloc(pool, sizeof(hdns_batch_resv_task_param_t));

    task_param->pool = pool;
    task_param->client = client;
    task_param->hosts = hdns_list_new(pool);
    hdns_list_dup(task_param->hosts, hosts, hdns_to_list_clone_fn_t(apr_pstrdup));
    task_param->query_type = query_type;
    task_param->client_ip = client_ip;
    task_param->using_cache = true;
    task_param->cb = cb;
    task_param->cb_param = cb_param;

    apr_status_t status = apr_thread_pool_push(g_hdns_api_thread_pool, hdns_batch_resv_task, task_param, 0, client);

    if (status != APR_SUCCESS) {
        hdns_pool_destroy(pool);
        return hdns_status_error(HDNS_RESOLVE_FAIL, HDNS_RESOLVE_FAIL_CODE, "Submit task failed",
                                 client->config->session_id);
    }
    return hdns_status_ok(client->config->session_id);
}

hdns_status_t hdns_get_results_for_hosts_async_without_cache(hdns_client_t *client,
                                                             const hdns_list_head_t *hosts,
                                                             hdns_query_type_t query_type,
                                                             const char *client_ip,
                                                             hdns_resv_done_callback_pt cb,
                                                             void *cb_param) {
    CHECK_HDNS_TASK_COUNT();
    if (NULL == cb) {
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "callback is null.",
                                 client->config->session_id);
    }
    if (hdns_list_is_empty(hosts)) {
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "hosts is empty.",
                                 client->config->session_id);
    }

    hdns_pool_new(pool);
    hdns_batch_resv_task_param_t *task_param = hdns_palloc(pool, sizeof(hdns_batch_resv_task_param_t));

    task_param->pool = pool;
    task_param->client = client;
    task_param->hosts = hdns_list_new(pool);
    hdns_list_dup(task_param->hosts, hosts, hdns_to_list_clone_fn_t(apr_pstrdup));
    task_param->query_type = query_type;
    task_param->client_ip = client_ip;
    task_param->using_cache = false;
    task_param->cb = cb;
    task_param->cb_param = cb_param;

    apr_status_t status = apr_thread_pool_push(g_hdns_api_thread_pool, hdns_batch_resv_task, task_param, 0, client);
    if (status != APR_SUCCESS) {
        hdns_pool_destroy(pool);
        return hdns_status_error(HDNS_RESOLVE_FAIL, HDNS_RESOLVE_FAIL_CODE, "Submit task failed",
                                 client->config->session_id);
    }
    return hdns_status_ok(client->config->session_id);
}

hdns_status_t hdns_log_set_log_file_path(const char *file_path) {
    apr_status_t s;
    char buf[256];
    apr_file_t *thefile;
    s = apr_file_open(&thefile, file_path,
                      APR_FOPEN_WRITE | APR_FOPEN_CREATE | APR_FOPEN_APPEND | APR_FOPEN_XTHREAD,
                      APR_UREAD | APR_UWRITE | APR_GREAD | APR_WREAD, g_hdns_api_pool);
    if (s != APR_SUCCESS) {
        hdns_log_error("apr_file_open failure, code:%d %s.", s, apr_strerror(s, buf, sizeof(buf)));
        return hdns_status_error(HDNS_OPEN_FILE_ERROR, HDNS_OPEN_FILE_ERROR_CODE, "log file open failed", NULL);
    }
    hdns_log_set_output(thefile);
    return hdns_status_ok(NULL);
}

void hdns_log_set_log_level(hdns_log_level_e level) {
    hdns_log_set_level(level);
}

void hdns_remove_host_cache(hdns_client_t *client, const char *host) {
    if (client != NULL && client->cache != NULL && hdns_str_is_not_blank(host)) {
        hdns_cache_table_delete(client->cache, host, HDNS_RR_TYPE_A);
        hdns_cache_table_delete(client->cache, host, HDNS_RR_TYPE_AAAA);
    }
}

static bool is_net_match(hdns_net_type_t net_type, hdns_rr_type_t rr_type) {
    switch (net_type) {
        case HDNS_DUAL_STACK: {
            return true;
        }
        case HDNS_IPV4_ONLY: {
            return rr_type == HDNS_RR_TYPE_A;
        }
        case HDNS_IPV6_ONLY: {
            return rr_type == HDNS_RR_TYPE_AAAA;
        }
        default: {
            return false;
        }
    }
}

static int hdns_select_ip(hdns_list_head_t *results, hdns_query_type_t query_type, char *ip, bool random) {
    if (hdns_list_is_empty(results)) {
        return HDNS_ERROR;
    }
    hdns_net_type_t net_type;
    switch (query_type) {
        case HDNS_QUERY_AUTO: {
            net_type = hdns_net_get_type(g_hdns_net_detector);
            if (net_type == HDNS_NET_UNKNOWN) {
                net_type = HDNS_IPV4_ONLY;
            }
            break;
        }
        case HDNS_QUERY_BOTH: {
            net_type = HDNS_DUAL_STACK;
            break;
        }
        case HDNS_QUERY_IPV6: {
            net_type = HDNS_IPV6_ONLY;
            break;
        }
        default: {
            net_type = HDNS_IPV4_ONLY;
        }
    }
    hdns_list_for_each_entry_safe(cursor, results) {
        hdns_resv_resp_t *resp = cursor->data;
        if (is_net_match(net_type, resp->type)) {
            if (hdns_list_is_not_empty(resp->ips)) {
                int index = random ? (rand() % hdns_list_size(resp->ips)) : 0;
                strcpy(ip, hdns_list_get(resp->ips, index));
                return HDNS_OK;
            }
        }
    }
    return HDNS_ERROR;
}


int hdns_select_ip_randomly(hdns_list_head_t *results, hdns_query_type_t query_type, char *ip) {
    return hdns_select_ip(results, query_type, ip, true);
}

int hdns_select_first_ip(hdns_list_head_t *results, hdns_query_type_t query_type, char *ip) {
    return hdns_select_ip(results, query_type, ip, false);
}


int hdns_get_sdns_extra(hdns_list_head_t *results, hdns_query_type_t query_type, char *extra) {
    if (hdns_list_is_empty(results)) {
        return HDNS_ERROR;
    }
    hdns_net_type_t net_type;
    switch (query_type) {
        case HDNS_QUERY_AUTO: {
            net_type = hdns_net_get_type(g_hdns_net_detector);
            if (net_type == HDNS_NET_UNKNOWN) {
                net_type = HDNS_IPV4_ONLY;
            }
            break;
        }
        case HDNS_QUERY_BOTH: {
            net_type = HDNS_DUAL_STACK;
            break;
        }
        case HDNS_QUERY_IPV6: {
            net_type = HDNS_IPV6_ONLY;
            break;
        }
        default: {
            net_type = HDNS_IPV4_ONLY;
        }
    }
    hdns_list_for_each_entry_safe(cursor, results) {
        hdns_resv_resp_t *resp = cursor->data;
        if (is_net_match(net_type, resp->type)) {
            if (hdns_str_is_not_blank(resp->extra)) {
                strcpy(extra, resp->extra);
                return HDNS_OK;
            }
        }
    }
    return HDNS_ERROR;
}

static void *APR_THREAD_FUNC hdns_client_cleanup_task(apr_thread_t *thread, void *data) {
    hdns_client_t *client = data;
    if (client != NULL) {
        // 停止该客户端关联的所有ip测速线程
        apr_thread_pool_tasks_cancel(g_hdns_api_thread_pool, client);
        // 停止该客户端关联的所有缓存刷新线程
        hdns_net_cancel_chg_cb_task(g_hdns_net_detector, client->cache);
        // 清理调度器相关资源
        hdns_scheduler_cleanup(client->scheduler);
        // 清理缓存相关资源
        hdns_cache_table_cleanup(client->cache);
        // 清理配置项相关资源
        hdns_config_cleanup(client->config);
        // 释放客户端内存池
        hdns_pool_destroy(client->pool);
        hdns_log_info("The client has been destroyed.");
    }
    return NULL;
}

void hdns_client_cleanup(hdns_client_t *client) {
    if (client != NULL) {
        client->state = HDNS_STATE_STOPPING;
        client->scheduler->state = HDNS_STATE_STOPPING;
        // 延迟30秒结束，等待正在执行的异步任务
        apr_thread_pool_schedule(g_hdns_api_thread_pool,
                                 hdns_client_cleanup_task,
                                 client,
                                 30 * APR_USEC_PER_SEC,
                                 NULL);
    }
}

void hdns_sdk_cleanup() {
    if (!g_hdns_api_initialized) {
        return;
    }
    if (--g_hdns_api_initialized) {
        return;
    }
    // 关停网络变化监测线程和测速分发线程
    hdns_net_detector_stop(g_hdns_net_detector);
    if (g_hdns_api_thread_pool != NULL) {
        apr_thread_pool_destroy(g_hdns_api_thread_pool);
        g_hdns_api_thread_pool = NULL;
    }
    hdns_net_detector_cleanup(g_hdns_net_detector);
    hdns_session_pool_cleanup();
    if (hdns_stdout_file != NULL) {
        apr_file_close(hdns_stdout_file);
        hdns_stdout_file = NULL;
    }
    hdns_log_cleanup();
    if (g_hdns_api_pool != NULL) {
        hdns_pool_destroy(g_hdns_api_pool);
        g_hdns_api_pool = NULL;
    }
    apr_terminate();
}