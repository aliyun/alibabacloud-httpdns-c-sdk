//
// Created by cagaoshuai on 2024/1/20.
//
#include "httpdns_resolver.h"
#include "httpdns_http.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"

static void setup(void) {
    init_httpdns_sdk();
}

static void teardown(void) {
    cleanup_httpdns_sdk();
}


static void on_http_finish_callback(char *response_body,
                                    int32_t response_status,
                                    int32_t response_rt_ms,
                                    void *user_callback_param) {
    log_trace("on_http_finish_callback, body=%s, response_status=%d, response_rt_ms=%d", response_body, response_status,
              response_rt_ms);
    (void) response_body;
    (void) response_rt_ms;
    bool *is_success = (bool *) (user_callback_param);
    *is_success = (HTTP_STATUS_OK == response_status);
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
        struct list_head *resolve_params,
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
    sds request_str = httpdns_resolve_request_to_string(request);
    log_trace("test_multi_resolve_task, request=%s", request_str);
    sdsfree(request_str);
    httpdns_resolve_request_free(request);
    httpdns_list_add(resolve_params, resolve_param, NULL);
}

START_TEST(test_single_resolve_task) {
    httpdns_config_t *config = get_httpdns_config();
    bool is_success = false;
    httpdns_resolve_request_t *request = httpdns_resolve_request_new(config,
                                                                     "www.aliyun.com,www.taobao.com",
                                                                     "203.107.1.1",
                                                                     HTTPDNS_QUERY_TYPE_BOTH);
    httpdns_resolve_request_set_using_multi(request, true);
    httpdns_resolve_param_t *resolve_param = build_resolve_param(request, &is_success);

    sds request_str = httpdns_resolve_request_to_string(request);
    log_trace("test_single_resolve_task, request=%s", request_str);
    sdsfree(request_str);

    httpdns_resolver_single_resolve(resolve_param);

    httpdns_resolve_request_free(request);
    httpdns_resolve_param_free(resolve_param);
    httpdns_config_free(config);
    ck_assert_msg(is_success, "单个解析请求执行失败");
}

END_TEST


START_TEST(test_multi_resolve_task) {
    httpdns_config_t *config = get_httpdns_config();
    bool is_success = false;
    NEW_EMPTY_LIST_IN_STACK(resolve_params);
    append_resolve_params(&resolve_params, config, "www.taobao.com", "203.107.1.1", &is_success);
    append_resolve_params(&resolve_params, config, "www.aliyun.com", "203.107.1.65", &is_success);
    httpdns_resolver_multi_resolve(&resolve_params);
    httpdns_list_free(&resolve_params, DATA_FREE_FUNC(httpdns_resolve_param_free));
    httpdns_config_free(config);
    ck_assert_msg(is_success, "批量解析请求执行失败");
}

END_TEST

Suite *make_httpdns_resolver_suite(void) {
    Suite *suite = suite_create("HTTPDNS Resolver Test");
    TCase *httpdns_resolver = tcase_create("httpdns_resolver");
    tcase_add_unchecked_fixture(httpdns_resolver, setup, teardown);
    suite_add_tcase(suite, httpdns_resolver);
    tcase_add_test(httpdns_resolver, test_single_resolve_task);
    tcase_add_test(httpdns_resolver, test_multi_resolve_task);
    return suite;
}