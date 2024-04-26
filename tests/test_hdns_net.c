//
// Created by caogaoshuai on 2024/1/14.
//

#include "hdns_net.h"
#include "test_suit_list.h"


void test_net_detect_task(CuTest *tc) {
    hdns_sdk_init();
    hdns_log_level = HDNS_LOG_DEBUG;
    hdns_client_t *client = hdns_client_create("139450", NULL);
    hdns_client_start(client);
    apr_sleep(2 * APR_USEC_PER_SEC);
    bool success = client->net_detector->type_detector->type != HDNS_NET_UNKNOWN;
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "网络类型探测任务不符合预期", success);
}

void test_net_detect_ipv4(CuTest *tc) {
    hdns_sdk_init();
    hdns_log_level = HDNS_LOG_DEBUG;
    hdns_client_t *client = hdns_client_create("139450", NULL);
    hdns_net_type_t net_type = hdns_net_get_type(client->net_detector);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "本地未发现ipv4网络", HDNS_IPV4_ONLY & net_type);
}


void test_net_detect_ipv6(CuTest *tc) {
    hdns_sdk_init();
    hdns_log_level = HDNS_LOG_DEBUG;
    hdns_client_t *client = hdns_client_create("139450", NULL);
    hdns_net_type_t net_type = hdns_net_get_type(client->net_detector);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "本地未发现ipv6网络", HDNS_IPV6_ONLY & net_type);
}

static void hdns_net_change_callback(void *param) {
    bool *success = param;
    *success = true;
}

void test_hdns_net_is_changed(CuTest *tc) {
    hdns_sdk_init();
    hdns_log_level = HDNS_LOG_DEBUG;
    hdns_client_t *client = hdns_client_create("139450", NULL);
    bool success = false;
    hdns_net_add_chg_cb_task(client->net_detector,
                             HDNS_NET_CB_UPDATE_CACHE,
                             (hdns_net_chg_cb_fn_t) hdns_net_change_callback,
                             &success,
                             client);
    // 需要切换网络
    apr_sleep(2 * APR_USEC_PER_SEC);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "网络变化监测失败", true);
}


void add_hdns_net_tests(CuSuite *suite) {
    SUITE_ADD_TEST(suite, test_net_detect_ipv4);
    SUITE_ADD_TEST(suite, test_net_detect_ipv6);
    SUITE_ADD_TEST(suite, test_net_detect_task);
    SUITE_ADD_TEST(suite, test_hdns_net_is_changed);
}


