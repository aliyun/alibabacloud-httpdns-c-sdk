//
// Created by cagaoshuai on 2024/1/20.
//
#include "httpdns_resolver.h"
#include "httpdns_http.h"

void on_http_finish_callback_func(char *response_body, int32_t response_status, int32_t response_rt_ms,
                                  void *user_callback_param) {
    printf("\nresponse(response_body=%s,response_status=%d,response_rt_ms=%d)", response_body, response_status,
           response_rt_ms);
    int32_t *ret = (int32_t *) user_callback_param;
    *ret = (HTTP_STATUS_OK == response_status);
}

static httpdns_resolve_param_t *get_test_param(httpdns_resolve_request_t *request, int *ret) {
    httpdns_resolve_param_t *resolve_param = httpdns_resolve_param_create(request);
    resolve_param->http_finish_callback_func = on_http_finish_callback_func;
    resolve_param->user_http_finish_callback_param = ret;
    return resolve_param;
}

static httpdns_config_t *get_test_config() {
    httpdns_config_t *config = create_httpdns_config();
    httpdns_config_set_account_id(config, "100000");
    httpdns_config_set_using_https(config, true);
    httpdns_config_set_secret_key(config, "29b79c1d12d6a30055f138a37f3f210f");
    httpdns_config_set_using_sign(config, true);
    httpdns_config_set_using_https(config, true);
    return config;
}

static int32_t test_single_resolve_task() {
    httpdns_config_t *config = get_test_config();
    int ret = HTTPDNS_SUCCESS;
    httpdns_resolve_request_t *request = httpdns_resolve_request_create(config,
                                                                        "www.aliyun.com,www.taobao.com",
                                                                        "203.107.1.65",
                                                                        HTTPDNS_QUERY_TYPE_BOTH);
    httpdns_resolve_request_set_using_multi(request, true);

    printf("\n");
    httpdns_resolve_request_print(request);

    httpdns_resolve_param_t *resolve_param = get_test_param(request, &ret);

    httpdns_resolver_single_resolve(resolve_param);
    httpdns_resolve_request_destroy(request);
    httpdns_resolve_param_destroy(resolve_param);
    destroy_httpdns_config(config);
    return ret;
}

static void
append_test_resolve_params(struct list_head *resolve_params, httpdns_config_t *config, char *host, char *resolver,
                           int *ret) {
    httpdns_resolve_request_t *request = httpdns_resolve_request_create(config, host, resolver,
                                                                        HTTPDNS_QUERY_TYPE_BOTH);
    httpdns_resolve_request_set_using_multi(request, true);
    printf("\n");
    httpdns_resolve_request_print(request);
    httpdns_resolve_param_t *resolve_param = get_test_param(request, ret);
    httpdns_resolve_request_destroy(request);
    httpdns_list_add(resolve_params, resolve_param, NULL);
}

static int32_t test_multi_resolve_task() {
    httpdns_config_t *config = get_test_config();
    int ret = HTTPDNS_SUCCESS;
    NEW_EMPTY_LIST_IN_STACK(resolve_params);
    append_test_resolve_params(&resolve_params, config, "www.taobao.com", "203.107.1.1", &ret);
    append_test_resolve_params(&resolve_params, config, "www.aliyun.com", "203.107.1.65", &ret);
    httpdns_resolver_multi_resolve(&resolve_params);
    httpdns_list_free(&resolve_params, DATA_FREE_FUNC(httpdns_resolve_param_destroy));
    destroy_httpdns_config(config);
    return ret;
}

int main(void) {
    int32_t success = HTTPDNS_SUCCESS;
    success |= test_single_resolve_task();
    success |= test_multi_resolve_task();
    return success;
}