//
// Created by caogaoshuai on 2024/1/20.
//
#include "httpdns_client.h"
#include "httpdns_list.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"

static httpdns_config_t *get_httpdns_config() {
    httpdns_config_t *config = httpdns_config_new();
    httpdns_config_set_account_id(config, "139450");
    httpdns_config_set_using_https(config, true);
    return config;
}

void test_simple_resolve_without_cache(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_config_t *config = get_httpdns_config();
    httpdns_config_set_using_cache(config, false);
    httpdns_client_t *client = httpdns_client_new(config);
    httpdns_resolve_result_t *result = NULL;
    httpdns_resolve_request_t *request = httpdns_resolve_request_new(config,
                                                                     "www.aliyun.com",
                                                                     NULL,
                                                                     HTTPDNS_QUERY_TYPE_BOTH);
    httpdns_client_simple_resolve(client, request, &result);
    httpdns_resolve_request_free(request);
    bool is_success = (NULL != result) && httpdns_list_is_not_empty(&result->ips);
    httpdns_resolve_result_free(result);
    httpdns_config_free(config);
    httpdns_client_free(client);
    cleanup_httpdns_sdk();
    CuAssert(tc, "简单解析接口解析失败", is_success);
}

void test_simple_resolve_with_cache(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_config_t *config = get_httpdns_config();
    httpdns_client_t *client = httpdns_client_new(config);
    httpdns_resolve_result_t *result = NULL;
    httpdns_resolve_request_t *request = httpdns_resolve_request_new(config,
                                                                     "www.taobao.com",
                                                                     NULL,
                                                                     HTTPDNS_QUERY_TYPE_BOTH);
    httpdns_resolve_request_set_using_cache(request, true);
    httpdns_client_simple_resolve(client, request, &result);
    httpdns_resolve_request_free(request);
    bool is_success = (NULL != result) && httpdns_list_is_not_empty(&result->ips);
    httpdns_resolve_result_free(result);
    sleep(1);
    request = httpdns_resolve_request_new(config,
                                          "www.taobao.com",
                                          NULL,
                                          HTTPDNS_QUERY_TYPE_BOTH);
    httpdns_resolve_request_set_using_cache(request, true);
    httpdns_client_simple_resolve(client, request, &result);
    httpdns_resolve_request_free(request);
    is_success = is_success && (NULL != result && result->hit_cache && httpdns_list_is_not_empty(&result->ips));
    httpdns_resolve_result_free(result);
    httpdns_config_free(config);
    httpdns_client_free(client);
    cleanup_httpdns_sdk();
    CuAssert(tc, "简单解析接口缓存未命中", is_success);
}


void test_multi_resolve_tasks(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_config_t *config = get_httpdns_config();
    httpdns_client_t *client = httpdns_client_new(config);
    httpdns_resolve_task_t *task = httpdns_resolve_task_new(client);

    httpdns_resolve_request_t *request = httpdns_resolve_request_new(
            config,
            "www.aliyun.com",
            NULL,
            HTTPDNS_QUERY_TYPE_BOTH);
    httpdns_resolve_task_add_request(task, request);
    httpdns_resolve_request_free(request);


    request = httpdns_resolve_request_new(
            config,
            "www.taobao.com",
            NULL,
            HTTPDNS_QUERY_TYPE_AUTO);
    httpdns_resolve_task_add_request(task, request);
    httpdns_resolve_request_free(request);


    request = httpdns_resolve_request_new(
            config,
            "www.google.com",
            NULL,
            HTTPDNS_QUERY_TYPE_A);
    httpdns_resolve_task_add_request(task, request);
    httpdns_resolve_request_free(request);

    httpdns_resolve_task_execute(task);

    size_t ctx_size = httpdns_list_size(&task->resolve_contexts);
    bool is_success = (ctx_size == 3);
    httpdns_list_for_each_entry(resolve_context_cursor, &task->resolve_contexts) {
        httpdns_resolve_context_t *resolve_context = resolve_context_cursor->data;
        if (NULL == resolve_context || httpdns_list_is_empty(&resolve_context->result)) {
            is_success = false;
        }
    }
    httpdns_resolve_task_free(task);
    httpdns_config_free(config);
    httpdns_client_free(client);
    cleanup_httpdns_sdk();
    CuAssert(tc, "校验多请求解析失败", is_success);
}


CuSuite *make_httpdns_client_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_simple_resolve_without_cache);
    SUITE_ADD_TEST(suite, test_simple_resolve_with_cache);
    SUITE_ADD_TEST(suite, test_multi_resolve_tasks);
    return suite;
}