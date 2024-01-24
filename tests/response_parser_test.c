//
// Created by cagaoshuai on 2024/1/19.
//

#include "httpdns_response.h"
#include <stdio.h>


/*
 报文示例：
{
	"service_ip":["203.107.1.65",
	"203.107.1.34",
	"203.107.1.66",
	"203.107.1.33",
	"203.107.1.1"],
	"service_ipv6":["2401:b180:2000:20::1c",
	"2401:b180:2000:20::10"]
}
 */
static void test_parse_schedule_result() {
    char *body = "{\n"
                 "\t\"service_ip\":[\"203.107.1.65\",\n"
                 "\t\"203.107.1.34\",\n"
                 "\t\"203.107.1.66\",\n"
                 "\t\"203.107.1.33\",\n"
                 "\t\"203.107.1.1\"],\n"
                 "\t\"service_ipv6\":[\"2401:b180:2000:20::1c\",\n"
                 "\t\"2401:b180:2000:20::10\"]\n"
                 "}";
    httpdns_schedule_response_t *response = httpdns_response_parse_schedule(body);
    printf("\n");
    httpdns_schedule_response_print(response);
    httpdns_schedule_response_destroy(response);
}

void test_parse_single_resolve_result() {
    char *body = "{\n"
                 "\"ipsv6\":[\"240e:960:c00:e:3:0:0:3ef\",\"240e:960:c00:e:3:0:0:3f0\"],\n"
                 "\"host\":\"www.aliyun.com\",\n"
                 "\"client_ip\":\"47.96.236.37\",\n"
                 "\"ips\":[\"47.118.227.108\",\"47.118.227.111\",\"47.118.227.112\"],\n"
                 "\"ttl\":60,\n"
                 "\"origin_ttl\":60\n"
                 "}";
    httpdns_single_resolve_response_t *response = httpdns_response_parse_single_resolve(body);
    printf("\n");
    httpdns_single_resolve_response_print(response);
    httpdns_single_resolve_response_destroy(response);
}

void test_parse_multi_resolve_result() {
    char* body = "{\n"
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
    printf("\n");
    httpdns_multi_resolve_response_print(response);
    httpdns_multi_resolve_response_destroy(response);
}

int main(void) {
    test_parse_schedule_result();
    test_parse_single_resolve_result();
    test_parse_multi_resolve_result();
    return 0;
}