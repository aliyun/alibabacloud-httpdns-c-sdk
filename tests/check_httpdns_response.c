//
// Created by cagaoshuai on 2024/1/19.
//

#include "http_response_parser.h"
#include "check_suit_list.h"


START_TEST(test_parse_schedule_response) {
    char *body = "{"
                 "\"service_ip\":[\"203.107.1.a\","
                 "\"203.107.1.b\","
                 "\"203.107.1.c\","
                 "\"203.107.1.d\","
                 "\"203.107.1.e\"],"
                 "\"service_ipv6\":[\"2401:b180:2000:20::hh\","
                 "\"2401:b180:2000:20::gg\"]"
                 "}";
    httpdns_schedule_response_t *response = httpdns_response_parse_schedule(body);
    bool is_expected = (NULL != response) && IS_NOT_EMPTY_LIST(&response->service_ip) &&
                       IS_NOT_EMPTY_LIST(&response->service_ipv6);

    sds parsed_body = httpdns_schedule_response_to_string(response);
    log_trace("test_parse_schedule_response, raw body=%s, parse result=%s", body, parsed_body);
    sdsfree(parsed_body);

    httpdns_schedule_response_free(response);
    ck_assert_msg(is_expected, "解析调度报文错误");
}

END_TEST

START_TEST(test_parse_single_resolve_response) {
    char *body = "{"
                 "\"ipsv6\":[\"240e:960:c00:e:3:0:0:3ef\",\"240e:960:c00:e:3:0:0:3f0\"],"
                 "\"host\":\"www.aliyun.com\","
                 "\"client_ip\":\"47.96.236.37\","
                 "\"ips\":[\"47.118.227.108\",\"47.118.227.111\",\"47.118.227.112\"],"
                 "\"ttl\":60,"
                 "\"origin_ttl\":60"
                 "}";
    httpdns_single_resolve_response_t *response = httpdns_response_parse_single_resolve(body);

    sds parsed_body = httpdns_single_resolve_response_to_string(response);
    log_trace("test_parse_single_resolve_response, raw body=%s, parse result=%s", body, parsed_body);
    sdsfree(parsed_body);

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
    char *body = "{"
                 "\"results\":[{"
                 "\"host\":\"www.aliyun.com\","
                 "\"client_ip\":\"47.96.236.37\","
                 "\"ips\":[\"47.118.227.116\"],"
                 "\"type\":1,"
                 "\"ttl\":26,"
                 "\"origin_ttl\":60"
                 "},"
                 "{"
                 "\"host\":\"www.taobao.com\","
                 "\"client_ip\":\"47.96.236.37\","
                 "\"ips\":[\"240e:f7:a093:101:3:0:0:3e8\"],"
                 "\"type\":28,"
                 "\"ttl\":60,"
                 "\"origin_ttl\":60"
                 "}]"
                 "}";
    httpdns_multi_resolve_response_t *response = httpdns_response_parse_multi_resolve(body);

    sds parsed_body = httpdns_multi_resolve_response_to_string(response);
    log_trace("test_parse_multi_resolve_response, raw body=%s, parse result=%s", body, parsed_body);
    sdsfree(parsed_body);

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