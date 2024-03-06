//
// Created by caogaoshuai on 2024/1/31.
//
#include "httpdns_client_wrapper.h"
#include "check_suit_list.h"
#include "httpdns_sds.h"
#include "httpdns_resolver.h"

static void httpdns_complete_callback_func(const httpdns_resolve_result_t *result, void *user_callback_param) {
    bool is_succes = NULL != result && httpdns_list_is_not_empty(&result->ips);
    int32_t *success_num = user_callback_param;
    if (is_succes) {
        *success_num = *success_num + 1;
    }


    httpdns_sds_t result_str = httpdns_resolve_result_to_string(result);
    httpdns_log_trace("callback result=%s", result_str);
    httpdns_sds_free(result_str);
}

void test_get_httpdns_result_for_host_sync(CuTest *tc) {
    httpdns_client_env_init("139450", NULL);
    httpdns_resolve_result_t *result = get_httpdns_result_for_host_sync_with_cache("www.aliyun.com",
                                                                                   HTTPDNS_QUERY_TYPE_AUTO,
                                                                                   NULL);
    httpdns_sds_t result_str = httpdns_resolve_result_to_string(result);
    httpdns_log_trace("test_get_httpdns_result_for_host_sync_with_cache, result %s", result_str);
    httpdns_sds_free(result_str);

    bool is_success = NULL != result && httpdns_list_is_not_empty(&result->ips);
    httpdns_resolve_result_free(result);
    httpdns_client_env_cleanup();
    CuAssert(tc, "同步且使用缓存的单解析失败", is_success);
}


void test_process_pre_resolve_hosts(CuTest *tc) {
    httpdns_client_env_init("139450", NULL);
    httpdns_config_t *httpdns_config = httpdns_client_get_config();
    httpdns_config_add_pre_resolve_host(httpdns_config,
                                        "www.aliyun.com");

    httpdns_client_process_pre_resolve_hosts();

    sleep(2);
    httpdns_resolve_result_t *result = get_httpdns_result_for_host_sync_with_cache("www.aliyun.com",
                                                                                   HTTPDNS_QUERY_TYPE_AUTO,
                                                                                   NULL);
    httpdns_sds_t result_str = httpdns_resolve_result_to_string(result);
    httpdns_log_trace("test_process_pre_resolve_hosts, result %s", result_str);
    httpdns_sds_free(result_str);

    bool is_success = NULL != result && result->hit_cache;
    httpdns_resolve_result_free(result);
    httpdns_client_env_cleanup();
    CuAssert(tc, "预加载处理失败", is_success);
}


void test_httpdns_sdns(CuTest *tc) {
    httpdns_client_env_init("139450", NULL);
    httpdns_config_t *httpdns_config = httpdns_client_get_config();
    httpdns_resolve_request_t *request = httpdns_resolve_request_new(httpdns_config,
                                                                     "httpdns.c.sdk.com",
                                                                     NULL,
                                                                     HTTPDNS_QUERY_TYPE_AUTO);
    httpdns_resolve_request_append_sdns_params(request,
                                               "a", "a");
    httpdns_resolve_result_t *result = get_httpdns_result_for_host_sync_with_custom_request(request);

    httpdns_sds_t result_str = httpdns_resolve_result_to_string(result);
    httpdns_log_trace("test_httpdns_sdns, result %s", result_str);
    httpdns_sds_free(result_str);

    bool is_success = NULL != result && httpdns_string_is_not_blank(result->extra);
    httpdns_resolve_result_free(result);
    httpdns_resolve_request_free(request);
    httpdns_client_env_cleanup();
    CuAssert(tc, "SDNS测试失败", is_success);
}


void test_get_httpdns_result_for_host_async(CuTest *tc) {
    httpdns_client_env_init("139450", NULL);
    int32_t success_num = 0;
    get_httpdns_result_for_host_async_with_cache("www.aliyun.com",
                                                 HTTPDNS_QUERY_TYPE_AUTO,
                                                 NULL,
                                                 httpdns_complete_callback_func,
                                                 &success_num
    );
    sleep(2);
    httpdns_client_env_cleanup();
    CuAssert(tc, "异步单解析失败", success_num == 1);
}


void test_get_httpdns_results_for_hosts_sync(CuTest *tc) {
    httpdns_client_env_init("139450", NULL);
    httpdns_list_new_empty_in_stack(hosts);
    httpdns_list_add(&hosts, "www.aliyun.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.taobao.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.baidu.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.google.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.freshippo.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.tmall.com", httpdns_string_clone_func);

    httpdns_list_new_empty_in_stack(results);
    int ret = get_httpdns_results_for_hosts_sync_with_cache(&hosts, HTTPDNS_QUERY_TYPE_AUTO, NULL, &results);

    httpdns_sds_t results_str = httpdns_list_to_string(&results,
                                                       to_httpdns_data_to_string_func(
                                                               httpdns_resolve_result_to_string));
    httpdns_log_trace("test_get_httpdns_results_for_hosts_sync_with_cache, results %s", results_str);
    httpdns_sds_free(results_str);

    bool is_success = HTTPDNS_SUCCESS == ret && httpdns_list_size(&results) == httpdns_list_size(&hosts);
    httpdns_list_free(&results, to_httpdns_data_free_func(httpdns_resolve_result_free));
    httpdns_list_free(&hosts, httpdns_string_free_func);
    httpdns_client_env_cleanup();
    CuAssert(tc, "同步批量解析失败", is_success);
}


void test_get_httpdns_results_for_hosts_async(CuTest *tc) {
    httpdns_client_env_init("139450", NULL);
    httpdns_list_new_empty_in_stack(hosts);
    httpdns_list_add(&hosts, "www.aliyun.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.taobao.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.baidu.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.google.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.freshippo.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.tmall.com", httpdns_string_clone_func);
    int32_t success_num = 0;
    get_httpdns_results_for_hosts_async_with_cache(&hosts,
                                                   HTTPDNS_QUERY_TYPE_AUTO,
                                                   NULL,
                                                   httpdns_complete_callback_func,
                                                   &success_num
    );
    sleep(5);
    bool is_success = success_num == httpdns_list_size(&hosts);
    httpdns_list_free(&hosts, httpdns_string_free_func);
    httpdns_client_env_cleanup();
    CuAssert(tc, "异步批量解析失败", is_success);
}


CuSuite *make_httpdns_client_wrapper_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_get_httpdns_result_for_host_sync);
    SUITE_ADD_TEST(suite, test_get_httpdns_result_for_host_async);
    SUITE_ADD_TEST(suite, test_get_httpdns_results_for_hosts_sync);
    SUITE_ADD_TEST(suite, test_get_httpdns_results_for_hosts_async);
    SUITE_ADD_TEST(suite, test_process_pre_resolve_hosts);
    SUITE_ADD_TEST(suite, test_httpdns_sdns);
    return suite;
}