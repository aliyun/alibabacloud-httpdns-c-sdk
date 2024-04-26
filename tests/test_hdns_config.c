//
// Created by caogaoshuai on 2024/1/15.
//

#include "hdns_config.h"
#include "test_suit_list.h"
#include "hdns_list.h"

void test_config_valid(CuTest *tc) {
    hdns_sdk_init();
    hdns_pool_new(pool);
    hdns_config_t *config = hdns_config_create(pool);
    config->account_id = apr_pstrdup(pool, "100000");
    hdns_status_t status = hdns_config_valid(config);
    hdns_pool_destroy(pool);
    hdns_sdk_cleanup();
    CuAssert(tc, "配置验证未通过", hdns_status_is_ok(&status));
}


void add_hdns_config_tests(CuSuite *suite) {
    SUITE_ADD_TEST(suite, test_config_valid);
}
