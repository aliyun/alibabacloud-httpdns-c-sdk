//
// Created by caogaoshuai on 2024/1/31.
//
#include "httpdns_client_wrapper.h"
#include "check_suit_list.h"
#include "httpdns_string.h"


static void setup(void) {
    httpdns_client_env_init("139450", NULL);
}

static void teardown(void) {
    httpdns_client_env_cleanup();
}

void httpdns_complete_callback_func(const httpdns_resolve_result_t *result, void *user_callback_param) {
    bool is_succes = NULL != result && httpdns_list_is_not_empty(&result->ips);
    int32_t *success_num = user_callback_param;
    if (is_succes) {
        *success_num = *success_num + 1;
    }


    sds result_str = httpdns_resolve_result_to_string(result);
    log_trace("callback result=%s", result_str);
    sdsfree(result_str);
}

START_TEST(test_get_httpdns_result_for_host_sync) {
    httpdns_resolve_result_t *result = get_httpdns_result_for_host_sync_with_cache("www.aliyun.com",
                                                                                   HTTPDNS_QUERY_TYPE_AUTO,
                                                                                   NULL);
    sds result_str = httpdns_resolve_result_to_string(result);
    log_trace("test_get_httpdns_result_for_host_sync_with_cache, result %s", result_str);
    sdsfree(result_str);

    bool is_success = NULL != result && httpdns_list_is_not_empty(&result->ips);
    httpdns_resolve_result_free(result);
    ck_assert_msg(is_success, "同步且使用缓存的单解析失败");
}

END_TEST

START_TEST(test_process_pre_resolve_hosts) {
    httpdns_config_t *httpdns_config = httpdns_client_get_config();
    httpdns_config_add_pre_resolve_host(httpdns_config, "www.aliyun.com");
    httpdns_client_process_pre_resolve_hosts();
    sleep(2);
    httpdns_resolve_result_t *result = get_httpdns_result_for_host_sync_with_cache("www.aliyun.com",
                                                                                   HTTPDNS_QUERY_TYPE_AUTO,
                                                                                   NULL);
    sds result_str = httpdns_resolve_result_to_string(result);
    log_trace("test_process_pre_resolve_hosts, result %s", result_str);
    sdsfree(result_str);

    bool is_success = NULL != result && result->hit_cache;
    httpdns_resolve_result_free(result);
    ck_assert_msg(is_success, "预加载处理失败");
}

END_TEST

START_TEST(test_httpdns_sdns) {
    httpdns_config_t *httpdns_config = httpdns_client_get_config();
    httpdns_resolve_request_t *request = httpdns_resolve_request_new(httpdns_config,
                                                                     "httpdns.c.sdk.com",
                                                                     NULL,
                                                                     HTTPDNS_QUERY_TYPE_AUTO);
    httpdns_resolve_request_append_sdns_params(request, "a", "a");
    httpdns_resolve_result_t *result = get_httpdns_result_for_host_sync_with_custom_request(request);

    sds result_str = httpdns_resolve_result_to_string(result);
    log_trace("test_httpdns_sdns, result %s", result_str);
    sdsfree(result_str);

    bool is_success = NULL != result && IS_NOT_BLANK_STRING(result->extra);
    httpdns_resolve_result_free(result);
    httpdns_resolve_request_free(request);
    ck_assert_msg(is_success, "SDNS测试失败");
}

END_TEST


START_TEST(test_get_httpdns_result_for_host_async) {
    int32_t success_num = 0;
    get_httpdns_result_for_host_async_with_cache("www.aliyun.com",
                                                 HTTPDNS_QUERY_TYPE_AUTO,
                                                 NULL,
                                                 httpdns_complete_callback_func,
                                                 &success_num
    );
    sleep(2);
    ck_assert_msg(success_num == 1, "异步单解析失败");
}

END_TEST


START_TEST(test_get_httpdns_results_for_hosts_sync) {
    httpdns_list_new_empty_in_stack(hosts);
    httpdns_list_add(&hosts, "www.aliyun.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.taobao.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.baidu.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.google.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.freshippo.com", httpdns_string_clone_func);
    httpdns_list_add(&hosts, "www.tmall.com", httpdns_string_clone_func);

    httpdns_list_new_empty_in_stack(results);
    int ret = get_httpdns_results_for_hosts_sync_with_cache(&hosts, HTTPDNS_QUERY_TYPE_AUTO, NULL, &results);

    sds results_str = httpdns_list_to_string(&results, to_httpdns_data_to_string_func(httpdns_resolve_result_to_string));
    log_trace("test_get_httpdns_results_for_hosts_sync_with_cache, results %s", results_str);
    sdsfree(results_str);

    bool is_success = HTTPDNS_SUCCESS == ret && httpdns_list_size(&results) == httpdns_list_size(&hosts);
    httpdns_list_free(&results, to_httpdns_data_free_func(httpdns_resolve_result_free));
    httpdns_list_free(&hosts, httpdns_string_free_func);
    ck_assert_msg(is_success, "同步批量解析失败");
}

END_TEST

START_TEST(test_get_httpdns_results_for_hosts_async) {
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
    ck_assert_msg(is_success, "异步批量解析失败");
}

END_TEST


Suite *make_httpdns_client_wrapper_suite(void) {
    Suite *suite = suite_create("HTTPDNS Client Wrapper Test");
    TCase *httpdns_client_wrapper = tcase_create("httpdns_client_wrapper");
    tcase_add_unchecked_fixture(httpdns_client_wrapper, setup, teardown);
    suite_add_tcase(suite, httpdns_client_wrapper);
    tcase_add_test(httpdns_client_wrapper, test_get_httpdns_result_for_host_sync);
    tcase_add_test(httpdns_client_wrapper, test_get_httpdns_result_for_host_async);
    tcase_add_test(httpdns_client_wrapper, test_get_httpdns_results_for_hosts_sync);
    tcase_add_test(httpdns_client_wrapper, test_get_httpdns_results_for_hosts_async);
    tcase_add_test(httpdns_client_wrapper, test_process_pre_resolve_hosts);
    tcase_add_test(httpdns_client_wrapper, test_httpdns_sdns);
    return suite;
}