//
// Created by caogaoshuai on 2024/1/15.
//

#include "httpdns_http.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"

static bool test_exchange_single_request(char *url) {
    init_httpdns_sdk();
    httpdns_http_context_t *http_context = httpdns_http_context_new(url, 10000);
    httpdns_http_single_exchange(http_context);
    httpdns_sds_t http_context_str = httpdns_http_context_to_string(http_context);
    httpdns_log_trace("test_exchange_single_request, http_context=%s", http_context_str);
    httpdns_sds_free(http_context_str);
    bool is_success = (NULL != http_context) && (http_context->response_status == HTTPDNS_HTTP_STATUS_OK);
    httpdns_http_context_free(http_context);
    return is_success;
}

void test_exchange_singel_request_with_resolve(CuTest *tc) {
    init_httpdns_sdk();
    char *url = "https://203.107.1.1/100000/d?host=www.aliyun.com";
    bool is_success = test_exchange_single_request(url);
    cleanup_httpdns_sdk();
    CuAssert(tc, "单HTTP请求-访问解析接口响应码非200", is_success);
}


void test_exchange_singel_request_with_schedule(CuTest *tc) {
    init_httpdns_sdk();
    char *url = "https://203.107.1.1/100000/ss";
    bool is_success = test_exchange_single_request(url);
    cleanup_httpdns_sdk();
    CuAssert(tc, "单HTTP请求-访问调度接口响应码非200", is_success);
}


void test_exchange_multi_request_with_resolve(CuTest *tc) {
    init_httpdns_sdk();
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
    cleanup_httpdns_sdk();
    CuAssert(tc, "批量HTTP接口访问存在失败", is_all_success);
}


CuSuite *make_httpdns_http_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_exchange_singel_request_with_resolve);
    SUITE_ADD_TEST(suite, test_exchange_singel_request_with_schedule);
    SUITE_ADD_TEST(suite, test_exchange_multi_request_with_resolve);
    return suite;
}