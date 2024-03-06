//
// Created by caogaoshuai on 2024/1/15.
//

#include "httpdns_client_config.h"
#include "check_suit_list.h"
#include "httpdns_list.h"
#include "httpdns_global_config.h"

void test_config_valid(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_config_t *config = httpdns_config_new();
    httpdns_config_set_account_id(config, "100000");
    bool is_expected = (httpdns_config_valid(config) == HTTPDNS_SUCCESS);
    httpdns_config_free(config);
    cleanup_httpdns_sdk();
    CuAssert(tc, "配置验证未通过", is_expected);

}

void test_add_pre_resolve_host(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_config_t *config = httpdns_config_new();
    httpdns_config_set_account_id(config, "100000");
    httpdns_config_add_pre_resolve_host(config, "www.baidu.com");
    httpdns_config_add_pre_resolve_host(config, "www.baidu.com");
    bool is_expected = (httpdns_config_valid(config) == HTTPDNS_SUCCESS)
                       && (httpdns_list_size(&config->pre_resolve_hosts) == 1);
    httpdns_config_free(config);
    cleanup_httpdns_sdk();
    CuAssert(tc, "预解析域名添加异常", is_expected);
}


CuSuite *make_httpdns_config_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_config_valid);
    SUITE_ADD_TEST(suite, test_add_pre_resolve_host);
    return suite;
}
