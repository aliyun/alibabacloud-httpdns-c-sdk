//
// Created by cagaoshuai on 2024/1/17.
//
#include "httpdns_scheduler.h"
#include "httpdns_list.h"
#include "check_suit_list.h"
#include "httpdns_ip.h"

START_TEST(test_refresh_resolve_servers) {
    httpdns_config_t *config = httpdns_config_new();
    httpdns_config_set_account_id(config, "100000");
    httpdns_config_set_using_https(config, true);
    httpdns_scheduler_t *scheduler = httpdns_scheduler_new(config);
    httpdns_scheduler_refresh(scheduler);
    bool is_success = httpdns_list_size(&scheduler->ipv4_resolve_servers) > 1
                      && httpdns_list_size(&scheduler->ipv6_resolve_servers) > 1;
    sds scheduler_str = httpdns_scheduler_to_string(scheduler);
    log_trace("test_refresh_resolve_servers, scheduler=%s", scheduler_str);
    sdsfree(scheduler_str);
    httpdns_config_free(config);
    httpdns_scheduler_free(scheduler);
    ck_assert_msg(is_success, "更新解析服务列表失败");
}

START_TEST(test_get_resolve_server) {
    httpdns_scheduler_t scheduler = {
            .net_stack_detector = httpdns_net_stack_detector_new()
    };
    httpdns_list_init(&scheduler.ipv4_resolve_servers);
    httpdns_list_init(&scheduler.ipv6_resolve_servers);
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
    sds scheduler_str = httpdns_scheduler_to_string(&scheduler);
    log_trace("test_get_resolve_server, get resolver %s from scheduler=%s", schedule_ip, scheduler_str);
    sdsfree(scheduler_str);
    httpdns_net_stack_detector_free(scheduler.net_stack_detector);
    sdsfree(schedule_ip);
    httpdns_list_free(&scheduler.ipv4_resolve_servers, NULL);
    ck_assert_msg(is_success, "未按照响应时间最小进行调度");
}

END_TEST

START_TEST(test_scheduler_update) {
    httpdns_scheduler_t scheduler;
    httpdns_list_init(&scheduler.ipv4_resolve_servers);
    httpdns_list_init(&scheduler.ipv6_resolve_servers);
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
    sds scheduler_str = httpdns_scheduler_to_string(&scheduler);
    log_trace("test_scheduler_update, before update %s, scheduler=%s", ip3.ip, scheduler_str);
    sdsfree(scheduler_str);
    httpdns_scheduler_update(&scheduler, "3.3.3.3", 100);
    scheduler_str = httpdns_scheduler_to_string(&scheduler);
    log_trace("test_scheduler_update, after update %s, scheduler=%s", ip3.ip, scheduler_str);
    sdsfree(scheduler_str);
    bool is_success = (ip3.rt == (int32_t)(DELTA_WEIGHT_UPDATE_RATION * 100 + (1 - DELTA_WEIGHT_UPDATE_RATION) * 35));
    httpdns_list_free(&scheduler.ipv4_resolve_servers, NULL);
    ck_assert_msg(is_success, "更新reslover响应时间失败");
}

END_TEST


Suite *make_httpdns_scheduler_suite(void) {
    Suite *suite = suite_create("HTTPDNS Scheduler Test");
    TCase *httpdns_scheduler = tcase_create("httpdns_scheduler");
    suite_add_tcase(suite, httpdns_scheduler);
    tcase_add_test(httpdns_scheduler, test_refresh_resolve_servers);
    tcase_add_test(httpdns_scheduler, test_get_resolve_server);
    tcase_add_test(httpdns_scheduler, test_scheduler_update);
    return suite;
}