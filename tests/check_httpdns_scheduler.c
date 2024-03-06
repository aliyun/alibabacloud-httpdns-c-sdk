//
// Created by caogaoshuai on 2024/1/17.
//
#include "httpdns_scheduler.h"
#include "httpdns_list.h"
#include "check_suit_list.h"
#include "httpdns_ip.h"
#include "pthread.h"
#include "httpdns_global_config.h"


void test_refresh_resolve_servers(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_config_t *config = httpdns_config_new();
    httpdns_config_set_account_id(config, "100000");
    httpdns_config_set_using_https(config, true);
    httpdns_scheduler_t *scheduler = httpdns_scheduler_new(config);
    httpdns_net_stack_detector_t *net_stack_detector = httpdns_net_stack_detector_new();
    httpdns_scheduler_set_net_stack_detector(scheduler, net_stack_detector);
    httpdns_scheduler_refresh(scheduler);
    bool is_success = httpdns_list_size(&scheduler->ipv4_resolve_servers) > 1
                      && httpdns_list_size(&scheduler->ipv6_resolve_servers) > 1;
    httpdns_sds_t scheduler_str = httpdns_scheduler_to_string(scheduler);
    httpdns_log_trace("test_refresh_resolve_servers, scheduler=%s", scheduler_str);
    httpdns_sds_free(scheduler_str);
    httpdns_config_free(config);
    httpdns_scheduler_free(scheduler);
    httpdns_net_stack_detector_free(net_stack_detector);
    cleanup_httpdns_sdk();
    CuAssert(tc, "更新解析服务列表失败", is_success);
}

void test_get_resolve_server(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_scheduler_t scheduler = {
            .net_stack_detector = httpdns_net_stack_detector_new()
    };
    httpdns_list_init(&scheduler.ipv4_resolve_servers);
    httpdns_list_init(&scheduler.ipv6_resolve_servers);
    pthread_mutexattr_init(&scheduler.lock_attr);
    pthread_mutexattr_settype(&scheduler.lock_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&scheduler.lock, &scheduler.lock_attr);
    httpdns_ip_t ip1 = {
            .ip = "1.1.1.1",
            .rt = 50
    };
    httpdns_ip_t ip2 = {
            .ip = "2.2.2.2",
            .rt = 20
    };
    httpdns_ip_t ip3 = {
            .ip = "3.3.3.3",
            .rt = 35
    };
    httpdns_list_add(&scheduler.ipv4_resolve_servers, &ip1, NULL);
    httpdns_list_add(&scheduler.ipv4_resolve_servers, &ip2, NULL);
    httpdns_list_add(&scheduler.ipv4_resolve_servers, &ip3, NULL);
    char *schedule_ip = httpdns_scheduler_get(&scheduler);
    bool is_success = strcmp("2.2.2.2", schedule_ip) == 0;
    httpdns_sds_t scheduler_str = httpdns_scheduler_to_string(&scheduler);
    httpdns_log_trace("test_get_resolve_server, get resolver %s from scheduler=%s", schedule_ip, scheduler_str);
    httpdns_sds_free(scheduler_str);
    httpdns_net_stack_detector_free(scheduler.net_stack_detector);
    httpdns_sds_free(schedule_ip);
    httpdns_list_free(&scheduler.ipv4_resolve_servers, NULL);
    pthread_mutex_destroy(&scheduler.lock);
    pthread_mutexattr_init(&scheduler.lock_attr);
    cleanup_httpdns_sdk();
    CuAssert(tc, "未按照响应时间最小进行调度", is_success);
}


void test_scheduler_update(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_scheduler_t scheduler;
    httpdns_list_init(&scheduler.ipv4_resolve_servers);
    httpdns_list_init(&scheduler.ipv6_resolve_servers);
    pthread_mutexattr_init(&scheduler.lock_attr);
    pthread_mutexattr_settype(&scheduler.lock_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&scheduler.lock, &scheduler.lock_attr);
    httpdns_ip_t ip1 = {
            .ip = "1.1.1.1",
            .rt = 50
    };
    httpdns_ip_t ip2 = {
            .ip = "2.2.2.2",
            .rt = 20
    };
    httpdns_ip_t ip3 = {
            .ip = "3.3.3.3",
            .rt = 35
    };
    httpdns_list_add(&scheduler.ipv4_resolve_servers, &ip1, NULL);
    httpdns_list_add(&scheduler.ipv4_resolve_servers, &ip2, NULL);
    httpdns_list_add(&scheduler.ipv4_resolve_servers, &ip3, NULL);
    httpdns_sds_t scheduler_str = httpdns_scheduler_to_string(&scheduler);
    httpdns_log_trace("test_scheduler_update, before update %s, scheduler=%s", ip3.ip, scheduler_str);
    httpdns_sds_free(scheduler_str);
    httpdns_scheduler_update(&scheduler, "3.3.3.3", 100);
    scheduler_str = httpdns_scheduler_to_string(&scheduler);
    httpdns_log_trace("test_scheduler_update, after update %s, scheduler=%s", ip3.ip, scheduler_str);
    httpdns_sds_free(scheduler_str);
    bool is_success = (ip3.rt == (int32_t) (HTTPDNS_DELTA_WEIGHT_UPDATE_RATION * 100 +
                                            (1 - HTTPDNS_DELTA_WEIGHT_UPDATE_RATION) * 35));
    httpdns_list_free(&scheduler.ipv4_resolve_servers, NULL);
    pthread_mutex_destroy(&scheduler.lock);
    pthread_mutexattr_init(&scheduler.lock_attr);
    cleanup_httpdns_sdk();
    CuAssert(tc, "更新reslover响应时间失败", is_success);
}


CuSuite *make_httpdns_scheduler_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_refresh_resolve_servers);
    SUITE_ADD_TEST(suite, test_get_resolve_server);
    SUITE_ADD_TEST(suite, test_scheduler_update);
    return suite;
}