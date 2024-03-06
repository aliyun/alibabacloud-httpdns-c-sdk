//
// Created by caogaoshuai on 2024/2/6.
//

#include "httpdns_list.h"
#include "httpdns_sds.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"
#include "httpdns_localdns.h"


void test_resolve_host_by_localdns(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_resolve_result_t *result = httpdns_localdns_resolve_host("www.aliyun.com");
    bool is_success = NULL != result && httpdns_list_is_not_empty(&result->ips);
    httpdns_sds_t resolve_result_str = httpdns_resolve_result_to_string(result);
    httpdns_log_trace("test_resolve_host_by_localdns result %s", resolve_result_str);
    httpdns_sds_free(resolve_result_str);
    httpdns_resolve_result_free(result);
    cleanup_httpdns_sdk();
    CuAssert(tc, "LocalDNS解析失败", is_success);
}


CuSuite *make_httpdns_localdns_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_resolve_host_by_localdns);
    return suite;
}