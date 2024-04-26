//
// Created by caogaoshuai on 2024/1/17.
//
#include "hdns_scheduler.h"
#include "hdns_list.h"
#include "test_suit_list.h"
#include "hdns_ip.h"
#include "hdns_api.h"


void test_refresh_resolve_servers(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);
    hdns_status_t status = hdns_scheduler_refresh_async(client->scheduler);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "更新解析服务列表失败", hdns_status_is_ok(&status));
}

void test_get_resolve_server(CuTest *tc) {
    hdns_sdk_init();
    //  签名测试已通过，secrete_key不对外透出，设置为NULL
    hdns_client_t *client = hdns_client_create("139450", NULL);
    hdns_list_free(client->scheduler->ipv4_resolvers);
    client->scheduler->ipv4_resolvers = hdns_list_new(NULL);
    hdns_ip_t ip1 = {
            .ip = "1.1.1.1",
            .rt = 50
    };
    hdns_ip_t ip2 = {
            .ip = "2.2.2.2",
            .rt = 20
    };
    hdns_ip_t ip3 = {
            .ip = "3.3.3.3",
            .rt = 35
    };
    hdns_list_add(client->scheduler->ipv4_resolvers, &ip1, NULL);
    hdns_list_add(client->scheduler->ipv4_resolvers, &ip2, NULL);
    hdns_list_add(client->scheduler->ipv4_resolvers, &ip3, NULL);

    char resolver[255];
    client->net_detector->type_detector->type = HDNS_IPV4_ONLY;
    hdns_scheduler_get(client->scheduler, resolver);
    bool success = strcmp("2.2.2.2", resolver) == 0;
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "未按照响应时间最小进行调度", success);
}


void test_scheduler_update(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_client_t *client = hdns_client_create("139450", NULL);

    hdns_list_free(client->scheduler->ipv4_resolvers);
    client->scheduler->ipv4_resolvers = hdns_list_new(NULL);

    hdns_ip_t ip1 = {
            .ip = "1.1.1.1",
            .rt = 50
    };
    hdns_ip_t ip2 = {
            .ip = "2.2.2.2",
            .rt = 20
    };
    hdns_ip_t ip3 = {
            .ip = "3.3.3.3",
            .rt = 35
    };

    hdns_list_add(client->scheduler->ipv4_resolvers, &ip1, NULL);
    hdns_list_add(client->scheduler->ipv4_resolvers, &ip2, NULL);
    hdns_list_add(client->scheduler->ipv4_resolvers, &ip3, NULL);
    hdns_scheduler_update(client->scheduler, "3.3.3.3", 100);
    hdns_log_debug("test_scheduler_update, after update %s, rt=%d", ip3.ip, ip3.rt);

    bool is_success = (ip3.rt == (int32_t) (HDNS_DELTA_WEIGHT_UPDATE_RATION * 100 +
                                            (1 - HDNS_DELTA_WEIGHT_UPDATE_RATION) * 35));
    hdns_sdk_cleanup();
    CuAssert(tc, "更新reslover响应时间失败", is_success);
}


void add_hdns_scheduler_tests(CuSuite *suite) {
    SUITE_ADD_TEST(suite, test_refresh_resolve_servers);
    SUITE_ADD_TEST(suite, test_get_resolve_server);
    SUITE_ADD_TEST(suite, test_scheduler_update);
}