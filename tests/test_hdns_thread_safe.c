//
// Created by caogaoshuai on 2024/1/20.
//
#include "hdns_api.h"
#include "test_suit_list.h"


typedef struct {
    hdns_client_t *client;
    bool success;
} hdns_test_task_param_t;

static void *
APR_THREAD_FUNC hdns_get_result_for_host_sync_with_custom_request_thread(apr_thread_t *thread, void *data) {
    hdns_test_task_param_t *param = data;
    hdns_cache_table_clean(param->client->cache);
    hdns_resv_req_t *req = hdns_resv_req_create(param->client);
    hdns_resv_req_append_sdns_param(req, "sdns-param1", "value1");
    hdns_resv_req_set_host(req, "httpdns.c.sdk.com");
    const char *cache_key = "cache_key";
    hdns_resv_req_set_cache_key(req, cache_key);
    hdns_resv_req_set_query_type(req, HDNS_QUERY_IPV4);

    hdns_list_head_t *results = NULL;

    size_t ips_size = 0;
    hdns_status_t s = hdns_get_result_for_host_sync_with_custom_request(param->client, req, &results);
    if (hdns_status_is_ok(&s)) {
        hdns_list_for_each_entry_safe(cursor, results) {
            hdns_resv_resp_t *resv_resp = cursor->data;
            char *resp_str = hdns_resv_resp_to_str(results->pool, resv_resp);
            hdns_log_debug("resp: %s", resp_str);
            ips_size += hdns_list_size(resv_resp->ips);
        }
    }

    bool hit_cache = false;
    hdns_resv_resp_t *resp = hdns_cache_table_get(param->client->cache, cache_key, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }
    hdns_list_free(results);
    param->success = hdns_status_is_ok(&s) && hit_cache && ips_size > 0;
    return NULL;
}

static void *
APR_THREAD_FUNC hdns_get_result_for_host_sync_with_cache_thread(apr_thread_t *thread, void *data) {
    hdns_test_task_param_t *param = data;
    hdns_cache_table_clean(param->client->cache);
    hdns_list_head_t *results = NULL;
    char *host = "www.aliyun.com";
    hdns_status_t s = hdns_get_result_for_host_sync_with_cache(param->client,
                                                               host,
                                                               HDNS_QUERY_AUTO,
                                                               NULL,
                                                               &results);

    size_t ips_size = 0;
    hdns_list_for_each_entry_safe(cursor, results) {
        hdns_resv_resp_t *resv_resp = cursor->data;
        char *resp_str = hdns_resv_resp_to_str(results->pool, resv_resp);
        hdns_log_debug("resp: %s", resp_str);
        ips_size += hdns_list_size(resv_resp->ips);
    }

    bool hit_cache = false;
    hdns_resv_resp_t *resp = hdns_cache_table_get(param->client->cache, host, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }
    resp = hdns_cache_table_get(param->client->cache, host, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }

    hdns_list_free(results);
    param->success = hdns_status_is_ok(&s) && hit_cache && ips_size > 0;
    return NULL;
}

static void *
APR_THREAD_FUNC hdns_get_result_for_host_sync_without_cache_thread(apr_thread_t *thread, void *data) {
    hdns_test_task_param_t *param = data;
    hdns_cache_table_clean(param->client->cache);
    hdns_list_head_t *results = NULL;

    char *host = "www.taobao.com";

    hdns_status_t s = hdns_get_result_for_host_sync_without_cache(param->client,
                                                                  host,
                                                                  HDNS_QUERY_AUTO,
                                                                  NULL,
                                                                  &results);
    size_t ips_size = 0;
    hdns_list_for_each_entry_safe(cursor, results) {
        hdns_resv_resp_t *resv_resp = cursor->data;
        char *resp_str = hdns_resv_resp_to_str(results->pool, resv_resp);
        hdns_log_debug("resp: %s", resp_str);
        ips_size += hdns_list_size(resv_resp->ips);
    }

    bool hit_cache = true;
    hdns_resv_resp_t *resp = hdns_cache_table_get(param->client->cache, host, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
    } else {
        hit_cache = false;
        hdns_log_debug("A Resource Record cache miss.");
    }
    resp = hdns_cache_table_get(param->client->cache, host, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
    } else {
        hit_cache = false;
        hdns_log_debug("AAAA Resource Record cache miss.");
    }

    hdns_list_free(results);
    param->success = hdns_status_is_ok(&s) && !hit_cache && ips_size > 0;
    return NULL;
}


static void *
APR_THREAD_FUNC  hdns_get_results_for_hosts_sync_with_cache_thread(apr_thread_t *thread, void *data) {
    hdns_test_task_param_t *param = data;
    hdns_cache_table_clean(param->client->cache);
    hdns_list_head_t *results = NULL;

    hdns_list_head_t *hosts = hdns_list_create();
    char *host1 = "www.taobao.com";
    char *host2 = "www.aliyun.com";

    hdns_list_add_str(hosts, host1);
    hdns_list_add_str(hosts, host2);

    hdns_status_t s = hdns_get_results_for_hosts_sync_with_cache(param->client,
                                                                 hosts,
                                                                 HDNS_QUERY_AUTO,
                                                                 NULL,
                                                                 &results);
    size_t ips_size = 0;
    hdns_list_for_each_entry_safe(cursor, results) {
        hdns_resv_resp_t *resv_resp = cursor->data;
        char *resp_str = hdns_resv_resp_to_str(results->pool, resv_resp);
        hdns_log_debug("resp: %s", resp_str);
        ips_size += hdns_list_size(resv_resp->ips);
    }

    bool hit_cache = false;
    hdns_resv_resp_t *resp = hdns_cache_table_get(param->client->cache, host1, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }
    resp = hdns_cache_table_get(param->client->cache, host2, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }

    hdns_list_free(results);

    param->success = hdns_status_is_ok(&s) && hit_cache && ips_size > 0;

    return NULL;
}


static void *
APR_THREAD_FUNC  hdns_get_results_for_hosts_sync_without_cache_thread(apr_thread_t *thread, void *data) {
    hdns_test_task_param_t *param = data;
    hdns_cache_table_clean(param->client->cache);
    hdns_list_head_t *results = NULL;

    hdns_list_head_t *hosts = hdns_list_create();
    char *host1 = "www.taobao.com";
    char *host2 = "www.aliyun.com";

    hdns_list_add_str(hosts, host1);
    hdns_list_add_str(hosts, host2);

    hdns_status_t s = hdns_get_results_for_hosts_sync_without_cache(param->client,
                                                                    hosts,
                                                                    HDNS_QUERY_AUTO,
                                                                    NULL,
                                                                    &results);
    size_t ips_size = 0;
    hdns_list_for_each_entry_safe(cursor, results) {
        hdns_resv_resp_t *resv_resp = cursor->data;
        char *resp_str = hdns_resv_resp_to_str(results->pool, resv_resp);
        hdns_log_debug("resp: %s", resp_str);
        ips_size += hdns_list_size(resv_resp->ips);
    }

    bool hit_cache = true;
    hdns_resv_resp_t *resp = hdns_cache_table_get(param->client->cache, host1, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    } else {
        hit_cache = false;
    }
    resp = hdns_cache_table_get(param->client->cache, host2, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    } else {
        hit_cache = false;
    }

    hdns_list_free(results);

    param->success = hdns_status_is_ok(&s) && !hit_cache && ips_size > 0;

    return NULL;
}

void hdns_resv_done_callback_func_in_multi_thread_safe(hdns_status_t *status, hdns_list_head_t *results, void *param) {
    if (hdns_status_is_ok(status)) {
        hdns_list_for_each_entry_safe(cursor, results) {
            hdns_resv_resp_t *resv_resp = cursor->data;
            char *resp_str = hdns_resv_resp_to_str(results->pool, resv_resp);
            hdns_log_debug("resp: %s", resp_str);
        }
        *((bool *) param) = true;
    }
}

static void *
APR_THREAD_FUNC  hdns_get_result_for_host_async_with_custom_request_thread(apr_thread_t *thread, void *data) {
    hdns_test_task_param_t *param = data;
    hdns_cache_table_clean(param->client->cache);
    hdns_resv_req_t *req = hdns_resv_req_create(param->client);

    hdns_resv_req_append_sdns_param(req, "sdns-param1", "value1");
    hdns_resv_req_set_host(req, "httpdns.c.sdk.com");

    bool sucess = false;
    hdns_get_result_for_host_async_with_custom_request(param->client,
                                                       req,
                                                       hdns_resv_done_callback_func_in_multi_thread_safe,
                                                       &sucess);
    apr_sleep(2 * APR_USEC_PER_SEC);


    bool hit_cache = false;
    hdns_resv_resp_t *resp = hdns_cache_table_get(param->client->cache, "httpdns.c.sdk.com", HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }

    param->success = sucess && hit_cache;

    return NULL;
}


static void *
APR_THREAD_FUNC hdns_get_result_for_host_async_with_cache_thread(apr_thread_t *thread, void *data) {
    hdns_test_task_param_t *param = data;
    hdns_cache_table_clean(param->client->cache);
    char *host = "www.aliyun.com";

    bool success = false;
    hdns_get_result_for_host_async_with_cache(param->client,
                                              host,
                                              HDNS_QUERY_AUTO,
                                              NULL,
                                              hdns_resv_done_callback_func_in_multi_thread_safe,
                                              &success);
    apr_sleep(2 * APR_USEC_PER_SEC);


    bool hit_cache = false;
    hdns_resv_resp_t *resp = hdns_cache_table_get(param->client->cache, host, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }
    resp = hdns_cache_table_get(param->client->cache, host, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }

    param->success = success && hit_cache;
}

static void *
APR_THREAD_FUNC hdns_get_result_for_host_async_without_cache_thread(apr_thread_t *thread, void *data) {
    hdns_test_task_param_t *param = data;
    hdns_cache_table_clean(param->client->cache);
    char *host = "www.taobao.com";
    bool sucess = false;
    hdns_get_result_for_host_async_without_cache(param->client,
                                                 "www.taobao.com",
                                                 HDNS_QUERY_AUTO,
                                                 NULL,
                                                 hdns_resv_done_callback_func_in_multi_thread_safe,
                                                 &sucess);

    apr_sleep(2 * APR_USEC_PER_SEC);


    bool hit_cache = true;
    hdns_resv_resp_t *resp = hdns_cache_table_get(param->client->cache, host, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
    } else {
        hit_cache = false;
        hdns_log_debug("A Resource Record cache miss.");
    }
    resp = hdns_cache_table_get(param->client->cache, host, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
    } else {
        hit_cache = false;
        hdns_log_debug("AAAA Resource Record cache miss.");
    }

    param->success = sucess && !hit_cache;
    return NULL;
}

static void *
APR_THREAD_FUNC  hdns_get_results_for_hosts_async_with_cache_thread(apr_thread_t *thread, void *data) {
    hdns_test_task_param_t *param = data;
    hdns_cache_table_clean(param->client->cache);
    hdns_list_head_t *hosts = hdns_list_create();
    char *host1 = "www.taobao.com";
    char *host2 = "www.aliyun.com";
    hdns_list_add_str(hosts, host1);
    hdns_list_add_str(hosts, host2);

    bool success = false;

    hdns_get_results_for_hosts_async_with_cache(param->client,
                                                hosts,
                                                HDNS_QUERY_AUTO,
                                                NULL,
                                                hdns_resv_done_callback_func_in_multi_thread_safe,
                                                &success);


    apr_sleep(2 * APR_USEC_PER_SEC);


    bool hit_cache = false;
    hdns_resv_resp_t *resp = hdns_cache_table_get(param->client->cache, host1, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }
    resp = hdns_cache_table_get(param->client->cache, host2, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }

    param->success = success && hit_cache;
    return NULL;
}


static void *
APR_THREAD_FUNC  hdns_get_results_for_hosts_async_without_cache_thread(apr_thread_t *thread, void *data) {
    hdns_test_task_param_t *param = data;
    hdns_cache_table_clean(param->client->cache);
    hdns_list_head_t *hosts = hdns_list_create();

    char *host1 = "www.taobao.com";
    char *host2 = "www.aliyun.com";
    hdns_list_add_str(hosts, host1);
    hdns_list_add_str(hosts, host2);

    bool success = false;

    hdns_get_results_for_hosts_async_without_cache(param->client,
                                                   hosts,
                                                   HDNS_QUERY_AUTO,
                                                   NULL,
                                                   hdns_resv_done_callback_func_in_multi_thread_safe,
                                                   &success);
    apr_sleep(2 * APR_USEC_PER_SEC);


    bool hit_cache = true;
    hdns_resv_resp_t *resp = hdns_cache_table_get(param->client->cache, host1, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    } else {
        hit_cache = false;
    }
    resp = hdns_cache_table_get(param->client->cache, host2, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    } else {
        hit_cache = false;
    }
    param->success = success && !hit_cache;
    return NULL;
}

void test_hdns_api_multi_threads(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);
    hdns_client_enable_update_cache_after_net_change(client, true);
    hdns_client_enable_expired_ip(client, true);
    char *pre_resolve_host = "www.taobao.com";
    hdns_client_add_pre_resolve_host(client, pre_resolve_host);
    hdns_client_add_ip_probe_item(client, pre_resolve_host, 443);
    hdns_client_start(client);

    // 创建线程池
    hdns_pool_new(pool);
    apr_thread_pool_t *thread_pool = NULL;
    apr_thread_pool_create(&thread_pool, 10, 16, pool);

    hdns_test_task_param_t *task_params[10];
    int arr_size = sizeof(task_params) / sizeof(task_params[0]);

    apr_thread_start_t *tasks[10] = {
            hdns_get_result_for_host_sync_with_custom_request_thread,
            hdns_get_result_for_host_sync_with_cache_thread,
            hdns_get_result_for_host_sync_without_cache_thread,
            hdns_get_results_for_hosts_sync_with_cache_thread,
            hdns_get_results_for_hosts_sync_without_cache_thread,
            hdns_get_result_for_host_async_with_custom_request_thread,
            hdns_get_result_for_host_async_with_cache_thread,
            hdns_get_result_for_host_async_without_cache_thread,
            hdns_get_results_for_hosts_async_with_cache_thread,
            hdns_get_results_for_hosts_async_without_cache_thread
    };

    char *msgs[10] = {
           "hdns_get_result_for_host_sync_with_custom_request_thread failed",
           "hdns_get_result_for_host_sync_with_cache_thread failed",
            "hdns_get_result_for_host_sync_without_cache_thread failed",
            "hdns_get_results_for_hosts_sync_with_cache_thread failed",
            "hdns_get_results_for_hosts_sync_without_cache_thread failed",
            "hdns_get_result_for_host_async_with_custom_request_thread failed",
            "hdns_get_result_for_host_async_with_cache_thread failed",
            "hdns_get_result_for_host_async_without_cache_thread failed",
            "hdns_get_results_for_hosts_async_with_cache_thread failed",
            "hdns_get_results_for_hosts_async_without_cache_thread failed"
    };

    for (int i = 0; i < arr_size; i++) {
        task_params[i] = hdns_palloc(pool, sizeof(hdns_test_task_param_t));
        task_params[i]->client = client;
        task_params[i]->success = false;
        apr_thread_pool_push(thread_pool, tasks[i], task_params[i], 0, client);
    }

    apr_sleep(5 * APR_USEC_PER_SEC);
    char msg[255] = "";
    for (int i = 0; i < arr_size; i++) {
        if (!task_params[i]->success) {
            strcpy(msg, msgs[i]);
            break;
        }
    }

    hdns_pool_destroy(pool);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, msg, hdns_str_is_blank(msg));
}


void add_hdns_thread_safe_tests(CuSuite *suite) {
    SUITE_ADD_TEST(suite, test_hdns_api_multi_threads);
}
