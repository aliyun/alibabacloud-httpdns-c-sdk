//
// Created by caogaoshuai on 2024/1/19.
//

#include "http_response_parser.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"

void test_parse_schedule_response(CuTest *tc) {
    init_httpdns_sdk();
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
    bool is_expected = (NULL != response) && httpdns_list_is_not_empty(&response->service_ip) &&
                       httpdns_list_is_not_empty(&response->service_ipv6);

    httpdns_sds_t parsed_body = httpdns_schedule_response_to_string(response);
    httpdns_log_trace("test_parse_schedule_response, raw body=%s, parse result=%s", body, parsed_body);
    httpdns_sds_free(parsed_body);

    httpdns_schedule_response_free(response);
    cleanup_httpdns_sdk();
    CuAssert(tc, "解析调度报文错误", is_expected);
}


void test_parse_single_resolve_response(CuTest *tc) {
    init_httpdns_sdk();
    char *body = "{"
                 "\"ipsv6\":[\"240e:960:c00:e:3:0:0:3ef\",\"240e:960:c00:e:3:0:0:3f0\"],"
                 "\"host\":\"www.aliyun.com\","
                 "\"client_ip\":\"47.96.236.37\","
                 "\"ips\":[\"47.118.227.108\",\"47.118.227.111\",\"47.118.227.112\"],"
                 "\"ttl\":60,"
                 "\"origin_ttl\":60"
                 "}";
    httpdns_single_resolve_response_t *response = httpdns_response_parse_single_resolve(body);

    httpdns_sds_t parsed_body = httpdns_single_resolve_response_to_string(response);
    httpdns_log_trace("test_parse_single_resolve_response, raw body=%s, parse result=%s", body, parsed_body);
    httpdns_sds_free(parsed_body);

    bool is_expected = (NULL != response)
                       && httpdns_list_is_not_empty(&response->ips)
                       && httpdns_list_is_not_empty(&response->ipsv6)
                       && (strcmp("www.aliyun.com", response->host) == 0)
                       && (response->ttl == 60)
                       && (response->origin_ttl == 60)
                       && (strcmp("47.96.236.37", response->client_ip) == 0);
    httpdns_single_resolve_response_free(response);
    cleanup_httpdns_sdk();
    CuAssert(tc, "单解析响应报文错误", is_expected);
}


void test_parse_multi_resolve_response(CuTest *tc) {
    init_httpdns_sdk();
    char *body = "{"
                 "\"dns\":[{"
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

    httpdns_sds_t parsed_body = httpdns_multi_resolve_response_to_string(response);
    httpdns_log_trace("test_parse_multi_resolve_response, raw body=%s, parse result=%s", body, parsed_body);
    httpdns_sds_free(parsed_body);

    bool is_expected = (NULL != response) && (httpdns_list_size(&response->dns) == 2);
    httpdns_multi_resolve_response_free(response);
    cleanup_httpdns_sdk();
    CuAssert(tc, "批量解析响应报文错误", is_expected);
}


CuSuite *make_httpdns_response_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_parse_schedule_response);
    SUITE_ADD_TEST(suite, test_parse_single_resolve_response);
    SUITE_ADD_TEST(suite, test_parse_multi_resolve_response);
    return suite;
}