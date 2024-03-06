//
// Created by caogaoshuai on 2024/1/19.
//
#include "httpdns_sign.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"

void test_signature_sign(CuTest *tc) {
    init_httpdns_sdk();
    struct timeval tv = {
            .tv_sec = 1706149424,
            .tv_usec = 0
    };
    httpdns_signature_t *signature = httpdns_signature_new("www.aliyun.com", "abcdef", 0, tv);
    bool is_matched = strcmp(signature->sign, "05a9283c8cfa35615bcd653cff3fac0f") == 0;
    if (NULL != signature) {
        httpdns_signature_free(signature);
    }
    cleanup_httpdns_sdk();
    CuAssert(tc, "签名验证失败", is_matched);
}

void test_signature_raw(CuTest *tc) {
    init_httpdns_sdk();
    struct timeval tv = {
            .tv_sec = 1706149424,
            .tv_usec = 0
    };
    httpdns_signature_t *signature = httpdns_signature_new("www.aliyun.com", "abcdef", 0, tv);
    bool is_matched = strcmp(signature->raw, "www.aliyun.com-abcdef-1706149424") == 0;
    if (NULL != signature) {
        httpdns_signature_free(signature);
    }
    cleanup_httpdns_sdk();
    CuAssert(tc, "签名原始字符串拼接不符合预期", is_matched);
}


void test_signature_timestamp(CuTest *tc) {
    init_httpdns_sdk();
    struct timeval tv = {
            .tv_sec = 1706149424,
            .tv_usec = 0
    };
    httpdns_signature_t *signature = httpdns_signature_new("www.aliyun.com", "abcdef", 0, tv);
    bool is_matched = strcmp(signature->timestamp, "1706149424") == 0;
    if (NULL != signature) {
        httpdns_signature_free(signature);
    }
    cleanup_httpdns_sdk();
    CuAssert(tc, "签名时间戳不符合预期", is_matched);
}


CuSuite *make_httpdns_sign_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_signature_sign);
    SUITE_ADD_TEST(suite, test_signature_timestamp);
    SUITE_ADD_TEST(suite, test_signature_raw);
    return suite;
}

