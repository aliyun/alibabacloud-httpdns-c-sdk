//
// Created by caogaoshuai on 2024/1/20.
//

#include "httpdns_time.h"
#include "check_suit_list.h"
#include "httpdns_sds.h"
#include "httpdns_global_config.h"


void test_to_string(CuTest *tc) {
    init_httpdns_sdk();
    struct timeval tv = {
            .tv_sec = 1706149424,
            .tv_usec = 0
    };

    httpdns_sds_t ts_str = httpdns_time_to_string(tv);
    httpdns_log_trace("test_to_string, time:%s", ts_str);
    bool is_matched = strcmp(ts_str, "2024-01-25 10:23:44") == 0;
    httpdns_sds_free(ts_str);
    cleanup_httpdns_sdk();
    CuAssert(tc, "日期格式化结果不符合预期", is_matched);
}

void test_now(CuTest *tc) {
    init_httpdns_sdk();
    struct timeval now1 = httpdns_time_now();
    struct timeval now2 = httpdns_time_now();
    httpdns_sds_t now1_str = httpdns_time_to_string(now1);
    httpdns_sds_t now2_str = httpdns_time_to_string(now1);
    httpdns_log_trace("test_now, now1_str=%s, now2_str=%s", now1_str, now2_str);
    httpdns_sds_free(now1_str);
    httpdns_sds_free(now2_str);
    cleanup_httpdns_sdk();
    CuAssert(tc, "时间差异过大", now2.tv_sec - now1.tv_sec < 1);
}

void test_is_expired(CuTest *tc) {
    struct timeval tv = {
            .tv_sec = 1706149424,
            .tv_usec = 0
    };
    CuAssert(tc, "测试过期时间失败", httpdns_time_is_expired(tv, 0));
    CuAssert(tc, "测试未过期时间失败", !httpdns_time_is_expired(tv, 5 * 365 * 24 * 60 * 60));
}


CuSuite *make_httpdns_time_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_to_string);
    SUITE_ADD_TEST(suite, test_now);
    SUITE_ADD_TEST(suite, test_is_expired);
    return suite;
}

