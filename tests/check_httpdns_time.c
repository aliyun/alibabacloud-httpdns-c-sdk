//
// Created by cagaoshuai on 2024/1/20.
//

#include "httpdns_time.h"
#include "check_suit_list.h"

START_TEST(test_to_string) {
    struct timeval tv = {
            .tv_sec = 1706149424,
            .tv_usec = 0
    };
    char time_str[100];
    httpdns_time_to_string(tv, time_str, 100);
    ck_assert_str_eq(time_str, "2024-01-25 10:23:44");
}

END_TEST

START_TEST(test_now) {
    struct timeval now1 = httpdns_time_now();
    struct timeval now2 = httpdns_time_now();
    ck_assert_msg(now2.tv_sec - now1.tv_sec < 1, "时间差异过大");
}

END_TEST

START_TEST(test_is_expired) {
    struct timeval tv = {
            .tv_sec = 1706149424,
            .tv_usec = 0
    };
    ck_assert_msg(httpdns_time_is_expired(tv, 0), "测试过期时间失败");
    ck_assert_msg(!httpdns_time_is_expired(tv, 5 * 365 * 24 * 60 * 60), "测试未过期时间失败");
}

END_TEST

Suite *make_httpdns_time_suite(void) {
    Suite *suite = suite_create("HTTPDNS Time Test");
    TCase *httpdns_time = tcase_create("httpdns_time");
    suite_add_tcase(suite, httpdns_time);
    tcase_add_test(httpdns_time, test_to_string);
    tcase_add_test(httpdns_time, test_now);
    tcase_add_test(httpdns_time, test_is_expired);
    return suite;
}

