//
// Created by caogaoshuai on 2024/1/15.
//

#include "httpdns_http.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"


static void setup(void) {
    init_httpdns_sdk();
}

static void teardown(void) {
    cleanup_httpdns_sdk();
}

static bool test_exchange_single_request(char *url) {
    httpdns_http_context_t *http_context = httpdns_http_context_new(url, 10000);
    httpdns_http_single_exchange(http_context);
    httpdns_sds_t http_context_str = httpdns_http_context_to_string(http_context);
    httpdns_log_trace("test_exchange_single_request, http_context=%s", http_context_str);
    httpdns_sds_free(http_context_str);
    bool is_success = (NULL != http_context) && (http_context->response_status == HTTPDNS_HTTP_STATUS_OK);
    httpdns_http_context_free(http_context);
    return is_success;
}

START_TEST(test_exchange_singel_request_with_resolve) {
    char *url = "https://203.107.1.1/100000/d?host=www.aliyun.com";
    bool is_success = test_exchange_single_request(url);
    ck_assert_msg(is_success, "单HTTP请求-访问解析接口响应码非200");
}

END_TEST

START_TEST(test_exchange_singel_request_with_schedule) {
    char *url = "https://203.107.1.1/100000/ss";
    bool is_success = test_exchange_single_request(url);
    ck_assert_msg(is_success, "单HTTP请求-访问调度接口响应码非200");
}

END_TEST


START_TEST(test_exchange_multi_request_with_resolve) {
    httpdns_list_new_empty_in_stack(http_contexts);
    httpdns_http_context_t *http_context = httpdns_http_context_new(
            "https://203.107.1.1/139450/d?host=www.baidu.com", 10000);
    httpdns_list_add(&http_contexts, http_context, NULL);

    http_context = httpdns_http_context_new(
            "https://203.107.1.1/139450/resolve?host=www.aliyun.com,qq.com,www.taobao.com,help.aliyun.com", 10000);
    httpdns_list_add(&http_contexts, http_context, NULL);

    http_context = httpdns_http_context_new(
            "https://203.107.1.1/139450/d?host=www.163.com", 10000);
    httpdns_list_add(&http_contexts, http_context, NULL);

    http_context = httpdns_http_context_new(
            "https://203.107.1.1/139450/d?host=huaweicloud.com", 10000);
    httpdns_list_add(&http_contexts, http_context, NULL);
    httpdns_http_multiple_exchange(&http_contexts);

    bool is_all_success = true;
    httpdns_list_for_each_entry(http_context_cursor, &http_contexts) {
        httpdns_http_context_t *ctx = http_context_cursor->data;
        httpdns_sds_t http_context_str = httpdns_http_context_to_string(ctx);
        httpdns_log_trace("test_exchange_multi_request_with_resolve, http_context=%s", http_context_str);
        httpdns_sds_free(http_context_str);
        if ((NULL == ctx) || (ctx->response_status != HTTPDNS_HTTP_STATUS_OK)) {
            is_all_success = false;
            break;
        }
    }
    httpdns_list_free(&http_contexts, to_httpdns_data_free_func(httpdns_http_context_free));
    ck_assert_msg(is_all_success, "批量HTTP接口访问存在失败");
}

END_TEST


Suite *make_httpdns_http_suite(void) {
    Suite *suite = suite_create("HTTPDNS HTTP Client Test");
    TCase *httpdns_http = tcase_create("httpdns_http");
    tcase_add_unchecked_fixture(httpdns_http, setup, teardown);
    suite_add_tcase(suite, httpdns_http);
    tcase_add_test(httpdns_http, test_exchange_singel_request_with_resolve);
    tcase_add_test(httpdns_http, test_exchange_singel_request_with_schedule);
    tcase_add_test(httpdns_http, test_exchange_multi_request_with_resolve);
    return suite;
}