//
// Created by caogaoshuai on 2024/1/14.
//

#include "httpdns_net_stack_detector.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"

static httpdns_net_stack_detector_t *net_detector = NULL;

static void setup(void) {
    init_httpdns_sdk();
    net_detector = httpdns_net_stack_detector_new();
}

static void teardown(void) {
    if (NULL != net_detector) {
        httpdns_net_stack_detector_free(net_detector);
    }
    cleanup_httpdns_sdk();
}

START_TEST(test_net_detect_ipv4) {
    net_stack_type_t net_type = httpdns_net_stack_type_get(net_detector);
    ck_assert_msg(httpdns_have_ipv4_net_type(net_type), "本地未发现ipv4网络");
}
END_TEST

START_TEST(test_net_detect_ipv6) {
    net_stack_type_t net_type = httpdns_net_stack_type_get(net_detector);
    ck_assert_msg(httpdns_have_ipv6_net_type(net_type), "本地未发现ipv6网络");
}
END_TEST


Suite *make_httpdns_net_suite(void) {
    Suite *suite = suite_create("HTTPDNS Network Stack Detector Test");
    TCase *httpdns_net = tcase_create("httpdns_net_stack_detector");
    tcase_add_unchecked_fixture(httpdns_net, setup, teardown);
    suite_add_tcase(suite, httpdns_net);
    tcase_add_test(httpdns_net, test_net_detect_ipv4);
    tcase_add_test(httpdns_net, test_net_detect_ipv6);
    return suite;
}


