//
// Created by cagaoshuai on 2024/1/20.
//
#include "httpdns_client.h"
#include "httpdns_list.h"
#include "check_suit_list.h"

static httpdns_config_t *get_httpdns_config() {
    httpdns_config_t *config = httpdns_config_create();
    httpdns_config_set_account_id(config, "100000");
    httpdns_config_set_using_https(config, true);
    return config;
}

START_TEST(test_simple_resolve_without_cache) {
    httpdns_config_t *config = get_httpdns_config();
    httpdns_config_set_using_cache(config, false);
    httpdns_client_t *client = httpdns_client_create(config);
    httpdns_resolve_result_t *result;
    httpdns_client_simple_resolve(client, "www.aliyun.com", HTTPDNS_QUERY_TYPE_BOTH, NULL, &result);
    bool is_success = (NULL != result) && IS_NOT_EMPTY_LIST(&result->ips);
    httpdns_resolve_result_destroy(result);
    httpdns_config_destroy(config);
    httpdns_client_destroy(client);
    ck_assert_msg(is_success, "简单解析接口解析失败");
}

START_TEST(test_simple_resolve_with_cache) {
    httpdns_config_t *config = get_httpdns_config();
    httpdns_client_t *client = httpdns_client_create(config);
    httpdns_resolve_result_t *result;
    httpdns_client_simple_resolve(client, "www.aliyun.com", HTTPDNS_QUERY_TYPE_BOTH, NULL, &result);
    bool is_success = (NULL != result) && IS_NOT_EMPTY_LIST(&result->ips);
    httpdns_resolve_result_destroy(result);
    sleep(1);
    httpdns_client_simple_resolve(client, "www.aliyun.com", HTTPDNS_QUERY_TYPE_BOTH, NULL, &result);
    is_success = is_success && (NULL != result && result->hit_cache && IS_NOT_EMPTY_LIST(&result->ips));
    httpdns_resolve_result_destroy(result);
    httpdns_config_destroy(config);
    httpdns_client_destroy(client);
    ck_assert_msg(is_success, "简单解析接口缓存未命中");
}

END_TEST

Suite *make_httpdns_client_suite(void) {
    Suite *suite = suite_create("HTTPDNS Client Test");
    TCase *httpdns_client = tcase_create("httpdns_client");
    suite_add_tcase(suite, httpdns_client);
    tcase_add_test(httpdns_client, test_simple_resolve_without_cache);
    tcase_add_test(httpdns_client, test_simple_resolve_with_cache);
    return suite;
}