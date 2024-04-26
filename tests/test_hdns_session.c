//
// Created by caogaoshuai on 2024/1/20.
//


#include "test_suit_list.h"
#include "hdns_session.h"


void hdns_test_get_session(CuTest *tc) {
    hdns_sdk_init();
    CURL *session = hdns_session_require();
    bool success = session != NULL;
    hdns_session_release(session);
    hdns_sdk_cleanup();
    CuAssert(tc, "获取Session失败", success);
}

void hdns_test_release_session(CuTest *tc) {
    hdns_sdk_init();
    CURL *session1 = hdns_session_require();
    hdns_session_release(session1);
    CURL *session2 = hdns_session_require();
    CURL *session3 = hdns_session_require();
    bool success = session1 == session2 && session2 != session3;
    hdns_session_release(session2);
    hdns_session_release(session3);
    hdns_sdk_cleanup();
    CuAssert(tc, "释放Session失败", success);
}


void add_hdns_session_tests(CuSuite *suite) {
    SUITE_ADD_TEST(suite, hdns_test_get_session);
    SUITE_ADD_TEST(suite, hdns_test_release_session);
}

