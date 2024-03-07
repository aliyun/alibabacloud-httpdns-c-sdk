//
// Created by caogaoshuai on 2024/1/14.
//

#include "httpdns_net_stack_detector.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"

void test_net_detect_ipv4(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_net_stack_detector_t *net_detector = httpdns_net_stack_detector_new();
    httpdns_net_stack_type_t net_type = httpdns_net_stack_type_get(net_detector);
    if (NULL != net_detector) {
        httpdns_net_stack_detector_free(net_detector);
    }
    cleanup_httpdns_sdk();
    CuAssert(tc, "本地未发现ipv4网络", httpdns_have_ipv4_net_type(net_type));
}


void test_net_detect_ipv6(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_net_stack_detector_t *net_detector = httpdns_net_stack_detector_new();
    httpdns_net_stack_type_t net_type = httpdns_net_stack_type_get(net_detector);
    if (NULL != net_detector) {
        httpdns_net_stack_detector_free(net_detector);
    }
    cleanup_httpdns_sdk();
    CuAssert(tc, "本地未发现ipv6网络", httpdns_have_ipv6_net_type(net_type));
}


CuSuite *make_httpdns_net_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_net_detect_ipv4);
    SUITE_ADD_TEST(suite, test_net_detect_ipv6);
    return suite;
}


