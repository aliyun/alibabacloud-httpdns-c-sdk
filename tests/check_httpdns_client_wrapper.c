//
// Created by cagaoshuai on 2024/1/31.
//
#include "httpdns_client_wrapper.h"
#include "check_suit_list.h"


static void setup(void) {
    httpdns_client_env_init("139450", NULL);
}

static void teardown(void) {
    httpdns_client_env_cleanup();
}

void httpdns_complete_callback_func(const httpdns_resolve_result_t *result, void *user_callback_param) {
    bool is_succes = NULL != result && IS_NOT_EMPTY_LIST(&result->ips);
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

    bool is_success = NULL != result && IS_NOT_EMPTY_LIST(&result->ips);
    httpdns_resolve_result_free(result);
    ck_assert_msg(is_success, "同步且使用缓存的单解析失败");
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


START_TEST(test_batch_get_httpdns_result_for_hosts_sync) {
    NEW_EMPTY_LIST_IN_STACK(hosts);
    httpdns_list_add(&hosts, "www.aliyun.com", STRING_CLONE_FUNC);
    httpdns_list_add(&hosts, "www.taobao.com", STRING_CLONE_FUNC);
    httpdns_list_add(&hosts, "www.baidu.com", STRING_CLONE_FUNC);
    httpdns_list_add(&hosts, "www.google.com", STRING_CLONE_FUNC);
    httpdns_list_add(&hosts, "www.freshippo.com", STRING_CLONE_FUNC);
    httpdns_list_add(&hosts, "www.tmall.com", STRING_CLONE_FUNC);

    NEW_EMPTY_LIST_IN_STACK(results);
    int ret = batch_get_httpdns_result_for_hosts_sync_with_cache(&hosts, HTTPDNS_QUERY_TYPE_AUTO, NULL, &results);

    sds results_str = httpdns_list_to_string(&results, DATA_TO_STRING_FUNC(httpdns_resolve_result_to_string));
    log_trace("test_batch_get_httpdns_result_for_hosts_sync_with_cache, results %s", results_str);
    sdsfree(results_str);

    bool is_success = HTTPDNS_SUCCESS == ret && httpdns_list_size(&results) == httpdns_list_size(&hosts);
    httpdns_list_free(&results, DATA_FREE_FUNC(httpdns_resolve_result_free));
    httpdns_list_free(&hosts, STRING_FREE_FUNC);
    ck_assert_msg(is_success, "同步批量解析失败");
}

END_TEST

START_TEST(test_batch_get_httpdns_result_for_hosts_async) {
    NEW_EMPTY_LIST_IN_STACK(hosts);
    httpdns_list_add(&hosts, "www.aliyun.com", STRING_CLONE_FUNC);
    httpdns_list_add(&hosts, "www.taobao.com", STRING_CLONE_FUNC);
    httpdns_list_add(&hosts, "www.baidu.com", STRING_CLONE_FUNC);
    httpdns_list_add(&hosts, "www.google.com", STRING_CLONE_FUNC);
    httpdns_list_add(&hosts, "www.freshippo.com", STRING_CLONE_FUNC);
    httpdns_list_add(&hosts, "www.tmall.com", STRING_CLONE_FUNC);
    int32_t success_num = 0;
    batch_get_httpdns_result_for_hosts_async_with_cache(&hosts,
                                                        HTTPDNS_QUERY_TYPE_AUTO,
                                                        NULL,
                                                        httpdns_complete_callback_func,
                                                        &success_num
    );
    sleep(5);
    bool is_success = success_num == httpdns_list_size(&hosts);
    httpdns_list_free(&hosts, STRING_FREE_FUNC);
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
    tcase_add_test(httpdns_client_wrapper, test_batch_get_httpdns_result_for_hosts_sync);
    tcase_add_test(httpdns_client_wrapper, test_batch_get_httpdns_result_for_hosts_async);
    return suite;
}