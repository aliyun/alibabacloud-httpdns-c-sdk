//
// Created by cagaoshuai on 2024/1/19.
//

#include "httpdns_http_response_parser.h"
#include "check_suit_list.h"


START_TEST(test_parse_schedule_response) {
    char *body = "{\n"
                 "\t\"service_ip\":[\"203.107.1.a\",\n"
                 "\t\"203.107.1.b\",\n"
                 "\t\"203.107.1.c\",\n"
                 "\t\"203.107.1.d\",\n"
                 "\t\"203.107.1.e\"],\n"
                 "\t\"service_ipv6\":[\"2401:b180:2000:20::hh\",\n"
                 "\t\"2401:b180:2000:20::gg\"]\n"
                 "}";
    httpdns_schedule_response_t *response = httpdns_response_parse_schedule(body);
    bool is_expected = (NULL != response) && IS_NOT_EMPTY_LIST(&response->service_ip) &&
                       IS_NOT_EMPTY_LIST(&response->service_ipv6);
    httpdns_schedule_response_free(response);
    ck_assert_msg(is_expected, "解析调度报文错误");
}

END_TEST

START_TEST(test_parse_single_resolve_response) {
    char *body = "{\n"
                 "\"ipsv6\":[\"240e:960:c00:e:3:0:0:3ef\",\"240e:960:c00:e:3:0:0:3f0\"],\n"
                 "\"host\":\"www.aliyun.com\",\n"
                 "\"client_ip\":\"47.96.236.37\",\n"
                 "\"ips\":[\"47.118.227.108\",\"47.118.227.111\",\"47.118.227.112\"],\n"
                 "\"ttl\":60,\n"
                 "\"origin_ttl\":60\n"
                 "}";
    httpdns_single_resolve_response_t *response = httpdns_response_parse_single_resolve(body);
    bool is_expected = (NULL != response)
                       && IS_NOT_EMPTY_LIST(&response->ips)
                       && IS_NOT_EMPTY_LIST(&response->ipsv6)
                       && (strcmp("www.aliyun.com", response->host) == 0)
                       && (response->ttl == 60)
                       && (response->origin_ttl == 60)
                       && (strcmp("47.96.236.37", response->client_ip) == 0);
    httpdns_single_resolve_response_free(response);
    ck_assert_msg(is_expected, "单解析响应报文错误");
}

END_TEST


START_TEST(test_parse_multi_resolve_response) {
    char *body = "{\n"
                 "\t\"results\":[{\n"
                 "\t\t\"host\":\"www.aliyun.com\",\n"
                 "\t\t\"client_ip\":\"47.96.236.37\",\n"
                 "\t\t\"ips\":[\"47.118.227.116\"],\n"
                 "\t\t\"type\":1,\n"
                 "\t\t\"ttl\":26,\n"
                 "\t\t\"origin_ttl\":60\n"
                 "\t},\n"
                 "\t{\n"
                 "\t\t\"host\":\"www.taobao.com\",\n"
                 "\t\t\"client_ip\":\"47.96.236.37\",\n"
                 "\t\t\"ips\":[\"240e:f7:a093:101:3:0:0:3e8\"],\n"
                 "\t\t\"type\":28,\n"
                 "\t\t\"ttl\":60,\n"
                 "\t\t\"origin_ttl\":60\n"
                 "\t}]\n"
                 "}";
    httpdns_multi_resolve_response_t *response = httpdns_response_parse_multi_resolve(body);
    bool is_expected = (NULL != response) && (httpdns_list_size(&response->dns) == 2);
    httpdns_multi_resolve_response_free(response);
    ck_assert_msg(is_expected, "批量解析响应报文错误");
}

END_TEST

Suite *make_httpdns_response_suite(void) {
    Suite *suite = suite_create("HTTPDNS Response Parser Test");
    TCase *httpdns_response = tcase_create("httpdns_response");
    suite_add_tcase(suite, httpdns_response);
    tcase_add_test(httpdns_response, test_parse_schedule_response);
    tcase_add_test(httpdns_response, test_parse_single_resolve_response);
    tcase_add_test(httpdns_response, test_parse_multi_resolve_response);
    return suite;
}