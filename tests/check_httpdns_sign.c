//
// Created by cagaoshuai on 2024/1/19.
//
#include "httpdns_sign.h"
#include "check_suit_list.h"

static httpdns_signature_t *signature = NULL;

static void setup(void) {
    struct timeval tv = {
            .tv_sec = 1706149424,
            .tv_usec = 0
    };
    signature = httpdns_signature_new("www.aliyun.com", "abcdef", 0, tv);
}

static void teardown(void) {
    if (NULL != signature) {
        httpdns_signature_free(signature);
    }
}

START_TEST(test_signature_sign) {
    ck_assert_str_eq(signature->sign, "05a9283c8cfa35615bcd653cff3fac0f");
}

END_TEST
START_TEST(test_signature_raw) {
    ck_assert_str_eq(signature->raw, "www.aliyun.com-abcdef-1706149424");
}

END_TEST

START_TEST(test_signature_timestamp) {
    ck_assert_str_eq(signature->timestamp, "1706149424");
}

END_TEST


Suite *make_httpdns_sign_suite(void) {
    Suite *suite = suite_create("HTTPDNS Signature Test");
    TCase *httpdns_sign = tcase_create("httpdns_sign");
    tcase_add_unchecked_fixture(httpdns_sign, setup, teardown);
    suite_add_tcase(suite, httpdns_sign);
    tcase_add_test(httpdns_sign, test_signature_raw);
    tcase_add_test(httpdns_sign, test_signature_timestamp);
    tcase_add_test(httpdns_sign, test_signature_sign);
    return suite;
}

