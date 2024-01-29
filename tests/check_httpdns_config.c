//
// Created by cagaoshuai on 2024/1/15.
//

#include "httpdns_client_config.h"
#include "check_suit_list.h"
#include "httpdns_list.h"


START_TEST(test_config_valid) {
    httpdns_config_t *config = httpdns_config_new();
    httpdns_config_set_account_id(config, "100000");
    bool is_expected = (httpdns_config_valid(config) == HTTPDNS_SUCCESS);
    httpdns_config_free(config);
    ck_assert_msg(is_expected, "配置验证未通过");

}

START_TEST(test_add_pre_resolve_host) {
    httpdns_config_t *config = httpdns_config_new();
    httpdns_config_set_account_id(config, "100000");
    httpdns_config_add_pre_resolve_host(config, "www.baidu.com");
    httpdns_config_add_pre_resolve_host(config, "www.baidu.com");
    bool is_expected = (httpdns_config_valid(config) == HTTPDNS_SUCCESS)
                       && (httpdns_list_size(&config->pre_resolve_hosts) == 1);
    httpdns_config_free(config);
    ck_assert_msg(is_expected, "预解析域名添加异常");
}

END_TEST

Suite *make_httpdns_config_suite(void) {
    Suite *suite = suite_create("HTTPDNS Config Test");
    TCase *httpdns_config = tcase_create("httpdns_config");
    suite_add_tcase(suite, httpdns_config);
    tcase_add_test(httpdns_config, test_config_valid);
    tcase_add_test(httpdns_config, test_add_pre_resolve_host);
    return suite;
}
