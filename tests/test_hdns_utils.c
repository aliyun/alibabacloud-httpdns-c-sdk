//
// Created by caogaoshuai on 2024/6/25.
//
#include "hdns_utils.h"
#include "test_suit_list.h"


void test_hdns_is_valid_ipv4(CuTest *tc) {
    hdns_sdk_init();
    bool success = hdns_is_valid_ipv4("1.1.1.1")
                   && !hdns_is_valid_ipv4("www.aliyun.com")
                   && !hdns_is_valid_ipv4("::1");
    hdns_sdk_cleanup();
    CuAssert(tc, "test_hdns_is_valid_ipv4 failed", success);
}

void test_hdns_is_valid_ipv6(CuTest *tc) {
    hdns_sdk_init();
    bool success = hdns_is_valid_ipv6("::1")
                   && !hdns_is_valid_ipv6("www.aliyun.com")
                   && !hdns_is_valid_ipv6("1.1.1.1");
    hdns_sdk_cleanup();
    CuAssert(tc, "test_hdns_is_valid_ipv6 failed", success);
}

void test_hdns_md5(CuTest *tc) {
    hdns_sdk_init();
    char *content = "app-api-uat-hk.lifebyte.dev-ee0bf1bfc8418bfaba63a39be3086b75-1719455770";
    char digest[64];
    hdns_md5(content, strlen(content), digest);
    bool success = (strcmp(digest, "680d99e0cf28f0b52ad585972d59f54c") == 0);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_hdns_md5 failed", success);
}

void test_hdns_encode_hex(CuTest *tc) {
    char *data = "a-bc";
    char hex[32];
    hdns_encode_hex(data, strlen(data), hex);
    bool success = (strcmp(hex, "612d6263") == 0);
    CuAssert(tc, "test_hdns_encode_hex failed", success);
}


void add_hdns_utils_tests(CuSuite *suite) {
    SUITE_ADD_TEST(suite, test_hdns_is_valid_ipv4);
    SUITE_ADD_TEST(suite, test_hdns_is_valid_ipv6);
    SUITE_ADD_TEST(suite, test_hdns_md5);
    SUITE_ADD_TEST(suite, test_hdns_encode_hex);
}