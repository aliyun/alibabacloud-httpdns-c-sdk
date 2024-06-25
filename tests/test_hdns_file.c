//
// Created by caogaoshuai on 2024/6/25.
//
#include "hdns_utils.h"
#include "hdns_file.h"
#include "test_suit_list.h"


void test_file_write(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_pool_new(pool);
    char *dir = apr_pstrcat(pool, get_user_home_dir(pool), "/.httpdns/", NULL);
    hdns_file_create_dir(dir);
    char *target_file_path = apr_pstrcat(pool, dir, "/server.json", NULL);
    apr_file_remove(target_file_path, pool);
    int32_t ret = hdns_file_write(target_file_path, "{\"hello\":123}");
    hdns_pool_destroy(pool);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_file_write failed", ret == HDNS_OK);
}

void test_file_read(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_pool_new(pool);
    char *dir = apr_pstrcat(pool, get_user_home_dir(pool), "/.httpdns/", NULL);
    hdns_file_create_dir(dir);
    char *target_file_path = apr_pstrcat(pool, dir, "/server.json", NULL);
    apr_file_remove(target_file_path, pool);
    hdns_file_write(target_file_path, "{\"hello\":123}");

    char *content = hdns_file_read(target_file_path, pool);
    bool is_not_blank = hdns_str_is_not_blank(content);
    hdns_pool_destroy(pool);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_file_read failed", is_not_blank);
}


void add_hdns_file_tests(CuSuite *suite) {
    SUITE_ADD_TEST(suite, test_file_write);
    SUITE_ADD_TEST(suite, test_file_read);
}