//
// Created by caogaoshuai on 2024/1/20.
//

#include "httpdns_time.h"
#include "check_suit_list.h"
#include "httpdns_sds.h"
#include "httpdns_global_config.h"


static void setup(void) {
    init_httpdns_sdk();
}

static void teardown(void) {
    cleanup_httpdns_sdk();
}

START_TEST(test_to_string) {
    struct timeval tv = {
            .tv_sec = 1706149424,
            .tv_usec = 0
    };

    httpdns_sds_t ts_str = httpdns_time_to_string(tv);
    log_trace("test_to_string, time:%s", ts_str);
    bool is_matched = strcmp(ts_str, "2024-01-25 10:23:44") == 0;
    httpdns_sds_free(ts_str);
    ck_assert_msg(is_matched, "日期格式化结果不符合预期");
}

END_TEST

START_TEST(test_now) {
    struct timeval now1 = httpdns_time_now();
    struct timeval now2 = httpdns_time_now();
    httpdns_sds_t now1_str = httpdns_time_to_string(now1);
    httpdns_sds_t now2_str = httpdns_time_to_string(now1);
    log_trace("test_now, now1_str=%s, now2_str=%s", now1_str, now2_str);
    httpdns_sds_free(now1_str);
    httpdns_sds_free(now2_str);
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
    tcase_add_unchecked_fixture(httpdns_time, setup, teardown);
    suite_add_tcase(suite, httpdns_time);
    tcase_add_test(httpdns_time, test_to_string);
    tcase_add_test(httpdns_time, test_now);
    tcase_add_test(httpdns_time, test_is_expired);
    return suite;
}

