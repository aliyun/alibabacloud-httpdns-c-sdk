//
// Created by cagaoshuai on 2024/1/20.
//
#include "httpdns_client.h"
#include "httpdns_list.h"
#include "check_suit_list.h"

static httpdns_config_t *get_httpdns_config() {
    httpdns_config_t *config = httpdns_config_new();
    httpdns_config_set_account_id(config, "139450");
    httpdns_config_set_using_https(config, true);
    return config;
}

START_TEST(test_simple_resolve_without_cache) {
    httpdns_config_t *config = get_httpdns_config();
    httpdns_config_set_using_cache(config, false);
    httpdns_client_t *client = httpdns_client_new(config);
    httpdns_resolve_result_t *result;
    httpdns_client_simple_resolve(client, "www.aliyun.com", HTTPDNS_QUERY_TYPE_BOTH, NULL, &result);
    bool is_success = (NULL != result) && IS_NOT_EMPTY_LIST(&result->ips);
    httpdns_resolve_result_free(result);
    httpdns_config_free(config);
    httpdns_client_free(client);
    ck_assert_msg(is_success, "简单解析接口解析失败");
}

START_TEST(test_simple_resolve_with_cache) {
    httpdns_config_t *config = get_httpdns_config();
    httpdns_client_t *client = httpdns_client_new(config);
    httpdns_resolve_result_t *result;
    httpdns_client_simple_resolve(client, "www.aliyun.com", HTTPDNS_QUERY_TYPE_BOTH, NULL, &result);
    bool is_success = (NULL != result) && IS_NOT_EMPTY_LIST(&result->ips);
    httpdns_resolve_result_free(result);
    sleep(1);
    httpdns_client_simple_resolve(client, "www.aliyun.com", HTTPDNS_QUERY_TYPE_BOTH, NULL, &result);
    is_success = is_success && (NULL != result && result->hit_cache && IS_NOT_EMPTY_LIST(&result->ips));
    httpdns_resolve_result_free(result);
    httpdns_config_free(config);
    httpdns_client_free(client);
    ck_assert_msg(is_success, "简单解析接口缓存未命中");
}

END_TEST

START_TEST(test_multi_resolve_task) {
    httpdns_config_t *config = get_httpdns_config();
    httpdns_client_t *client = httpdns_client_new(config);
    httpdns_resolve_task_t *task = httpdns_resolve_task_new(client);

    httpdns_resolve_request_t *request = httpdns_resolve_request_create(
            config,
            "www.aliyun.com",
            NULL,
            HTTPDNS_QUERY_TYPE_BOTH);
    httpdns_resolve_task_add_request(task, request);
    httpdns_resolve_request_free(request);


    request = httpdns_resolve_request_create(
            config,
            "www.taobao.com",
            NULL,
            HTTPDNS_QUERY_TYPE_AUTO);
    httpdns_resolve_task_add_request(task, request);
    httpdns_resolve_request_free(request);


    request = httpdns_resolve_request_create(
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
        if (NULL == resolve_context || IS_EMPTY_LIST(&resolve_context->result)) {
            is_success = false;
        }
    }
    httpdns_resolve_task_free(task);
    httpdns_config_free(config);
    httpdns_client_free(client);
    ck_assert_msg(is_success, "校验多请求解析失败");
}

END_TEST

Suite *make_httpdns_client_suite(void) {
    Suite *suite = suite_create("HTTPDNS Client Test");
    TCase *httpdns_client = tcase_create("httpdns_client");
    suite_add_tcase(suite, httpdns_client);
    tcase_add_test(httpdns_client, test_simple_resolve_without_cache);
    tcase_add_test(httpdns_client, test_simple_resolve_with_cache);
    tcase_add_test(httpdns_client, test_multi_resolve_task);
    return suite;
}