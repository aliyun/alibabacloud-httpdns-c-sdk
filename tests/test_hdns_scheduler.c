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
    hdns_client_t *client = hdns_client_create(HDNS_TEST_ACCOUNT, HDNS_TEST_SECRET_KEY);
    hdns_status_t status = hdns_scheduler_refresh_async(client->scheduler);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_refresh_resolve_servers failed", hdns_status_is_ok(&status));
}

void test_get_resolve_server(CuTest *tc) {
    hdns_sdk_init();
    //  签名测试已通过，secrete_key不对外透出，设置为NULL
    hdns_client_t *client = hdns_client_create(HDNS_TEST_ACCOUNT, HDNS_TEST_SECRET_KEY);
    hdns_list_free(client->scheduler->ipv4_resolvers);
    client->scheduler->ipv4_resolvers = hdns_list_new(NULL);
    hdns_list_add(client->scheduler->ipv4_resolvers, "2.2.2.2", NULL);
    hdns_list_add(client->scheduler->ipv4_resolvers, "3.3.3.3", NULL);
    hdns_list_add(client->scheduler->ipv4_resolvers, "4.4.4.4", NULL);

    char resolver[255];
    client->net_detector->type_detector->type = HDNS_IPV4_ONLY;
    hdns_scheduler_get(client->scheduler, resolver);
    bool success = strcmp("2.2.2.2", resolver) == 0;

    hdns_scheduler_failover(client->scheduler, "2.2.2.2");

    hdns_scheduler_get(client->scheduler, resolver);
    success = success && (strcmp("3.3.3.3", resolver) == 0);

    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_get_resolve_server failed", success);
}

void add_hdns_scheduler_tests(CuSuite *suite) {
    SUITE_ADD_TEST(suite, test_refresh_resolve_servers);
    SUITE_ADD_TEST(suite, test_get_resolve_server);
}