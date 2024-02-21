//
// Created by caogaoshuai on 2024/2/6.
//

#include "httpdns_list.h"
#include "httpdns_sds.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"
#include "httpdns_localdns.h"

static void setup(void) {
    init_httpdns_sdk();
}

static void teardown(void) {
    cleanup_httpdns_sdk();
}

START_TEST(test_resolve_host_by_localdns) {
    httpdns_resolve_result_t *result = httpdns_localdns_resolve_host("www.aliyun.com");
    bool is_success = NULL != result && httpdns_list_is_not_empty(&result->ips);
    httpdns_sds_t resolve_result_str = httpdns_resolve_result_to_string(result);
    log_trace("test_resolve_host_by_localdns result %s", resolve_result_str);
    httpdns_sds_free(resolve_result_str);
    httpdns_resolve_result_free(result);
    ck_assert_msg(is_success, "LocalDNS解析失败");
}

END_TEST


Suite *make_httpdns_localdns_suite(void) {
    Suite *suite = suite_create("HTTPDNS Localdns Test");
    TCase *httpdns_localdns = tcase_create("httpdns_localdns");
    tcase_add_unchecked_fixture(httpdns_localdns, setup, teardown);
    suite_add_tcase(suite, httpdns_localdns);
    tcase_add_test(httpdns_localdns, test_resolve_host_by_localdns);
    return suite;
}