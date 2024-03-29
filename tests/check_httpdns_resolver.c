//
// Created by caogaoshuai on 2024/1/20.
//
#include "httpdns_resolver.h"
#include "httpdns_http.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"

static void on_http_finish_callback(httpdns_http_context_t *http_context,
                                    void *user_callback_param) {
    httpdns_sds_t http_context_str = httpdns_http_context_to_string(http_context);
    httpdns_log_trace("on_http_finish_callback, %s", http_context_str);
    httpdns_sds_free(http_context_str);

    bool *is_success = (bool *) (user_callback_param);
    *is_success = (HTTPDNS_HTTP_STATUS_OK == http_context->response_status);
}

static httpdns_resolve_param_t *build_resolve_param(httpdns_resolve_request_t *request, bool *is_success) {
    httpdns_resolve_param_t *resolve_param = httpdns_resolve_param_new(request);
    resolve_param->http_complete_callback_func = on_http_finish_callback;
    resolve_param->user_http_complete_callback_param = is_success;
    return resolve_param;
}

static httpdns_config_t *get_httpdns_config() {
    httpdns_config_t *config = httpdns_config_new();
    httpdns_config_set_account_id(config, "100000");
    httpdns_config_set_using_https(config, true);
    return config;
}

static void append_resolve_params(
        httpdns_list_head_t *resolve_params,
        httpdns_config_t *config,
        char *host,
        char *resolver,
        bool *is_success) {
    httpdns_resolve_request_t *request = httpdns_resolve_request_new(
            config,
            host,
            resolver,
            HTTPDNS_QUERY_TYPE_BOTH);
    httpdns_resolve_request_set_using_multi(request, true);
    httpdns_resolve_param_t *resolve_param = build_resolve_param(request, is_success);
    httpdns_sds_t request_str = httpdns_resolve_request_to_string(request);
    httpdns_log_trace("test_multi_resolve_task, request=%s", request_str);
    httpdns_sds_free(request_str);
    httpdns_resolve_request_free(request);
    httpdns_list_add(resolve_params, resolve_param, NULL);
}

void test_single_resolve_task(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_config_t *config = get_httpdns_config();
    bool is_success = false;
    httpdns_resolve_request_t *request = httpdns_resolve_request_new(config,
                                                                     "www.aliyun.com,www.taobao.com",
                                                                     "203.107.1.1",
                                                                     HTTPDNS_QUERY_TYPE_BOTH);
    httpdns_resolve_request_set_using_multi(request, true);
    httpdns_resolve_param_t *resolve_param = build_resolve_param(request, &is_success);

    httpdns_sds_t request_str = httpdns_resolve_request_to_string(request);
    httpdns_log_trace("test_single_resolve_task, request=%s", request_str);
    httpdns_sds_free(request_str);

    httpdns_resolver_single_resolve(resolve_param);

    httpdns_resolve_request_free(request);
    httpdns_resolve_param_free(resolve_param);
    httpdns_config_free(config);
    cleanup_httpdns_sdk();
    CuAssert(tc, "单个解析请求执行失败", is_success);
}


void test_multi_resolve_task(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_config_t *config = get_httpdns_config();
    bool is_success = false;
    httpdns_list_new_empty_in_stack(resolve_params);
    append_resolve_params(&resolve_params, config, "www.taobao.com", "203.107.1.1", &is_success);
    append_resolve_params(&resolve_params, config, "www.aliyun.com", "203.107.1.65", &is_success);
    httpdns_resolver_multi_resolve(&resolve_params);
    httpdns_list_free(&resolve_params, to_httpdns_data_free_func(httpdns_resolve_param_free));
    httpdns_config_free(config);
    cleanup_httpdns_sdk();
    CuAssert(tc, "批量解析请求执行失败", is_success);
}


CuSuite *make_httpdns_resolver_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_single_resolve_task);
    SUITE_ADD_TEST(suite, test_multi_resolve_task);
    return suite;
}