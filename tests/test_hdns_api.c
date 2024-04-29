//
// Created by caogaoshuai on 2024/1/20.
//
#include "hdns_api.h"
#include "test_suit_list.h"


void test_pre_reslove_hosts(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);
    char *pre_resolve_host = "www.taobao.com";
    hdns_client_add_pre_resolve_host(client, pre_resolve_host);

    hdns_status_t status = hdns_client_start(client);

    apr_sleep(2 * APR_USEC_PER_SEC);

    bool hit_cache = false;
    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, pre_resolve_host, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get pre-resove resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }
    resp = hdns_cache_table_get(client->cache, pre_resolve_host, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get pre-resove resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }

    hdns_client_cleanup(client);
    hdns_sdk_cleanup();

    CuAssert(tc, "test_pre_reslove_hosts failed", hdns_status_is_ok(&status) && hit_cache);
}

void test_hdns_get_result_for_host_sync_with_custom_request(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);
    hdns_client_start(client);

    hdns_resv_req_t *req = hdns_resv_req_create(client);
    hdns_resv_req_append_sdns_param(req, "sdns-param1", "value1");
    hdns_resv_req_set_host(req, "httpdns.c.sdk.com");
    const char *cache_key = "cache_key";
    hdns_resv_req_set_cache_key(req, cache_key);
    hdns_resv_req_set_query_type(req, HDNS_QUERY_IPV4);

    hdns_list_head_t *results = NULL;

    size_t ips_size = 0;
    hdns_status_t s = hdns_get_result_for_host_sync_with_custom_request(client, req, &results);
    if (hdns_status_is_ok(&s)) {
        hdns_list_for_each_entry_safe(cursor, results) {
            hdns_resv_resp_t *resv_resp = cursor->data;
            char *resp_str = hdns_resv_resp_to_str(results->pool, resv_resp);
            hdns_log_debug("resp: %s", resp_str);
            ips_size += hdns_list_size(resv_resp->ips);
        }
    }

    bool hit_cache = false;
    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, cache_key, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }

    hdns_list_free(results);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();

    CuAssert(tc, "test_hdns_get_result_for_host_sync_with_custom_request failed", hdns_status_is_ok(&s) && hit_cache && ips_size > 0);
}


void test_hdns_get_result_for_host_sync_with_cache(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);

    hdns_list_head_t *results = NULL;
    char *host = "www.aliyun.com";
    hdns_status_t s = hdns_get_result_for_host_sync_with_cache(client,
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
    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, host, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }
    resp = hdns_cache_table_get(client->cache, host, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }

    hdns_list_free(results);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();

    CuAssert(tc, "test_hdns_get_result_for_host_sync_with_cache failed", hdns_status_is_ok(&s) && hit_cache && ips_size > 0);
}

void test_hdns_get_result_for_host_sync_without_cache(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);

    hdns_list_head_t *results = NULL;

    char *host = "www.taobao.com";

    hdns_status_t s = hdns_get_result_for_host_sync_without_cache(client,
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
    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, host, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
    } else {
        hit_cache = false;
        hdns_log_debug("A Resource Record cache miss.");
    }
    resp = hdns_cache_table_get(client->cache, host, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
    } else {
        hit_cache = false;
        hdns_log_debug("AAAA Resource Record cache miss.");
    }

    hdns_list_free(results);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();

    CuAssert(tc, "test_hdns_get_result_for_host_sync_without_cache failed", hdns_status_is_ok(&s) && !hit_cache && ips_size > 0);
}


void test_hdns_get_results_for_hosts_sync_with_cache(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);

    hdns_list_head_t *results = NULL;

    hdns_list_head_t *hosts = hdns_list_create();
    char *host1 = "www.taobao.com";
    char *host2 = "www.aliyun.com";

    hdns_list_add_str(hosts, host1);
    hdns_list_add_str(hosts, host2);

    hdns_status_t s = hdns_get_results_for_hosts_sync_with_cache(client,
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
    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, host1, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }
    resp = hdns_cache_table_get(client->cache, host2, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }

    hdns_list_free(results);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();

    CuAssert(tc, "test_hdns_get_results_for_hosts_sync_with_cache failed", hdns_status_is_ok(&s) && hit_cache && ips_size > 0);
}


void test_hdns_get_results_for_hosts_sync_without_cache(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);

    hdns_list_head_t *results = NULL;

    hdns_list_head_t *hosts = hdns_list_create();
    char *host1 = "www.taobao.com";
    char *host2 = "www.aliyun.com";

    hdns_list_add_str(hosts, host1);
    hdns_list_add_str(hosts, host2);

    hdns_status_t s = hdns_get_results_for_hosts_sync_without_cache(client,
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
    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, host1, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    } else {
        hit_cache = false;
    }
    resp = hdns_cache_table_get(client->cache, host2, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    } else {
        hit_cache = false;
    }

    hdns_list_free(results);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();

    CuAssert(tc, "test_hdns_get_results_for_hosts_sync_without_cache failed", hdns_status_is_ok(&s) && !hit_cache && ips_size > 0);
}

void hdns_resv_done_callback_func(hdns_status_t *status, hdns_list_head_t *results, void *param) {
    if (hdns_status_is_ok(status)) {
        hdns_list_for_each_entry_safe(cursor, results) {
            hdns_resv_resp_t *resv_resp = cursor->data;
            char *resp_str = hdns_resv_resp_to_str(results->pool, resv_resp);
            hdns_log_debug("resp: %s", resp_str);
        }
        *((bool *) param) = true;
    }
}

void test_hdns_get_result_for_host_async_with_custom_request(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);

    hdns_resv_req_t *req = hdns_resv_req_create(client);
    hdns_client_start(client);

    hdns_resv_req_append_sdns_param(req, "sdns-param1", "value1");
    hdns_resv_req_set_host(req, "httpdns.c.sdk.com");

    bool sucess = false;
    hdns_get_result_for_host_async_with_custom_request(client,
                                                       req,
                                                       hdns_resv_done_callback_func,
                                                       &sucess);
    apr_sleep(2 * APR_USEC_PER_SEC);


    bool hit_cache = false;
    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, "httpdns.c.sdk.com", HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }


    hdns_client_cleanup(client);
    hdns_sdk_cleanup();

    CuAssert(tc, "test_hdns_get_result_for_host_async_with_custom_request failed", sucess && hit_cache);
}


void test_hdns_get_result_for_host_async_with_cache(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);

    char *host = "www.aliyun.com";

    bool success = false;
    hdns_get_result_for_host_async_with_cache(client,
                                              host,
                                              HDNS_QUERY_AUTO,
                                              NULL,
                                              hdns_resv_done_callback_func,
                                              &success);
    apr_sleep(2 * APR_USEC_PER_SEC);


    bool hit_cache = false;
    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, host, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }
    resp = hdns_cache_table_get(client->cache, host, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }

    hdns_client_cleanup(client);
    hdns_sdk_cleanup();

    CuAssert(tc, "test_hdns_get_result_for_host_async_with_cache failed", success && hit_cache);
}

void test_hdns_get_result_for_host_async_without_cache(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);


    char *host = "www.taobao.com";
    bool sucess = false;
    hdns_get_result_for_host_async_without_cache(client,
                                                 "www.taobao.com",
                                                 HDNS_QUERY_AUTO,
                                                 NULL,
                                                 hdns_resv_done_callback_func,
                                                 &sucess);

    apr_sleep(2 * APR_USEC_PER_SEC);


    bool hit_cache = true;
    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, host, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
    } else {
        hit_cache = false;
        hdns_log_debug("A Resource Record cache miss.");
    }
    resp = hdns_cache_table_get(client->cache, host, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
    } else {
        hit_cache = false;
        hdns_log_debug("AAAA Resource Record cache miss.");
    }

    hdns_client_cleanup(client);
    hdns_sdk_cleanup();

    CuAssert(tc, "test_hdns_get_result_for_host_async_without_cache failed", sucess && !hit_cache);
}

void test_hdns_get_results_for_hosts_async_with_cache(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);

    hdns_list_head_t *hosts = hdns_list_create();
    char *host1 = "www.taobao.com";
    char *host2 = "www.aliyun.com";
    hdns_list_add_str(hosts, host1);
    hdns_list_add_str(hosts, host2);

    bool success = false;

    hdns_get_results_for_hosts_async_with_cache(client,
                                                hosts,
                                                HDNS_QUERY_AUTO,
                                                NULL,
                                                hdns_resv_done_callback_func,
                                                &success);


    apr_sleep(2 * APR_USEC_PER_SEC);


    bool hit_cache = false;
    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, host1, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }
    resp = hdns_cache_table_get(client->cache, host2, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }


    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_hdns_get_results_for_hosts_async_with_cache failed", success && hit_cache);
}


void test_hdns_get_results_for_hosts_async_without_cache(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);

    hdns_list_head_t *hosts = hdns_list_create();

    char *host1 = "www.taobao.com";
    char *host2 = "www.aliyun.com";
    hdns_list_add_str(hosts, host1);
    hdns_list_add_str(hosts, host2);

    bool success = false;

    hdns_get_results_for_hosts_async_without_cache(client,
                                                   hosts,
                                                   HDNS_QUERY_AUTO,
                                                   NULL,
                                                   hdns_resv_done_callback_func,
                                                   &success);
    apr_sleep(2 * APR_USEC_PER_SEC);


    bool hit_cache = true;
    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, host1, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    } else {
        hit_cache = false;
    }
    resp = hdns_cache_table_get(client->cache, host2, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    } else {
        hit_cache = false;
    }


    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_hdns_get_results_for_hosts_async_without_cache failed", success && !hit_cache);
}

void test_hdns_log(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_status_t s = hdns_log_set_log_file_path("/tmp/httpdns.log");
    hdns_log_info("test httpdns log");
    hdns_sdk_cleanup();
    CuAssert(tc, "test_hdns_log failed", hdns_status_is_ok(&s));
}

void test_clean_host_cache(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);
    char *pre_resolve_host = "www.aliyun.com";
    hdns_client_add_pre_resolve_host(client, pre_resolve_host);
    hdns_client_start(client);
    apr_sleep(2 * APR_USEC_PER_SEC);

    bool hit_cache = false;
    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, pre_resolve_host, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get pre-resove resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }
    resp = hdns_cache_table_get(client->cache, pre_resolve_host, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get pre-resove resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    }

    hdns_remove_host_cache(client, pre_resolve_host);

    bool clean_success = false;
    resp = hdns_cache_table_get(client->cache, pre_resolve_host, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get pre-resove resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
    } else {
        clean_success = true;
    }
    resp = hdns_cache_table_get(client->cache, pre_resolve_host, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get pre-resove resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
        hit_cache = true;
    } else {
        clean_success = true;
    }

    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_clean_host_cache failed", hit_cache && clean_success);
}


void test_hdns_client_enable_update_cache_after_net_change(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
//    hdns_log_set_log_file_path("/tmp/httpdns.log");
    hdns_client_t *client = hdns_client_create("139450", NULL);
    hdns_client_enable_update_cache_after_net_change(client, true);
    hdns_client_enable_expired_ip(client, true);
    hdns_client_set_using_https(client, false);

    char *pre_resolve_host = "www.aliyun.com";
    hdns_client_add_pre_resolve_host(client, pre_resolve_host);
    hdns_client_start(client);

    apr_sleep(2 * APR_USEC_PER_SEC);
    // 测试时需要长时间阻塞网络，手动切换网络环境
//    apr_sleep(3000 * APR_USEC_PER_SEC);

    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, pre_resolve_host, HDNS_RR_TYPE_A);

    bool success = (resp != NULL);
    hdns_resv_resp_destroy(resp);

    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_hdns_client_enable_update_cache_after_net_change failed", success);
}


void test_hdns_client_ip_probe(CuTest *tc) {
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

    apr_sleep(10 * APR_USEC_PER_SEC);

    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, pre_resolve_host, HDNS_RR_TYPE_A);

    bool success = (resp != NULL);
    hdns_resv_resp_destroy(resp);

    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_hdns_client_ip_probe failed", success);
}

void test_hdns_client_failover_localdns(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    const char *invalid_account_id = "000000";
    hdns_client_t *client = hdns_client_create(invalid_account_id, NULL);
    hdns_client_enable_failover_localdns(client, true);

    hdns_client_start(client);

    hdns_list_head_t *results = NULL;
    char *host = "www.aliyun.com";
    hdns_status_t s = hdns_get_result_for_host_sync_with_cache(client,
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
    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, host, HDNS_RR_TYPE_A);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
    } else {
        hit_cache = false;
    }
    resp = hdns_cache_table_get(client->cache, host, HDNS_RR_TYPE_AAAA);
    if (resp != NULL) {
        hdns_log_debug("get result from cache resp:%s", hdns_resv_resp_to_str(resp->pool, resp));
        hdns_resv_resp_destroy(resp);
    } else {
        hit_cache = false;
    }

    hdns_list_free(results);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();

    CuAssert(tc, "test_hdns_client_failover_localdns failed", hdns_status_is_ok(&s) && !hit_cache && ips_size > 0);
}


void test_hdns_client_add_custom_ttl(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    const char *invalid_account_id = "139450";
    hdns_client_t *client = hdns_client_create(invalid_account_id, NULL);

    char *host = "www.aliyun.com";

    int custom_ttl = 1000;
    hdns_client_add_custom_ttl_item(client, host, custom_ttl);

    hdns_client_start(client);

    hdns_list_head_t *results = NULL;

    hdns_status_t s = hdns_get_result_for_host_sync_with_cache(client,
                                                               host,
                                                               HDNS_QUERY_AUTO,
                                                               NULL,
                                                               &results);
    int expected_ttl = custom_ttl - 100;
    size_t ips_size = 0;
    hdns_list_for_each_entry_safe(cursor, results) {
        hdns_resv_resp_t *resv_resp = cursor->data;
        char *resp_str = hdns_resv_resp_to_str(results->pool, resv_resp);
        hdns_log_debug("resp: %s", resp_str);
        ips_size += hdns_list_size(resv_resp->ips);
        expected_ttl = resv_resp->origin_ttl;
    }

    hdns_list_free(results);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();

    CuAssert(tc, "test_hdns_client_add_custom_ttl failed", hdns_status_is_ok(&s) && (expected_ttl == custom_ttl) && ips_size > 0);
}


void add_hdns_api_tests(CuSuite *suite) {
    /*SUITE_ADD_TEST(suite, test_pre_reslove_hosts);
    SUITE_ADD_TEST(suite, test_hdns_get_result_for_host_sync_with_custom_request);
    SUITE_ADD_TEST(suite, test_hdns_get_result_for_host_sync_with_cache);
    SUITE_ADD_TEST(suite, test_hdns_get_result_for_host_sync_without_cache);
    SUITE_ADD_TEST(suite, test_hdns_get_results_for_hosts_sync_with_cache);
    SUITE_ADD_TEST(suite, test_hdns_get_results_for_hosts_sync_without_cache);
    SUITE_ADD_TEST(suite, test_hdns_get_result_for_host_async_with_custom_request);
    SUITE_ADD_TEST(suite, test_hdns_get_result_for_host_async_with_cache);
    SUITE_ADD_TEST(suite, test_hdns_get_result_for_host_async_without_cache);
    SUITE_ADD_TEST(suite, test_hdns_get_results_for_hosts_async_with_cache);
    SUITE_ADD_TEST(suite, test_hdns_get_results_for_hosts_async_without_cache);
    SUITE_ADD_TEST(suite, test_hdns_log);
    SUITE_ADD_TEST(suite, test_clean_host_cache);
    SUITE_ADD_TEST(suite, test_hdns_client_enable_update_cache_after_net_change);
    SUITE_ADD_TEST(suite, test_hdns_client_ip_probe);*/
    SUITE_ADD_TEST(suite, test_hdns_client_failover_localdns);
    SUITE_ADD_TEST(suite, test_hdns_client_add_custom_ttl);
}
