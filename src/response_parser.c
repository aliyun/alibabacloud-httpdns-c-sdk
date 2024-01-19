//
// Created by cagaoshuai on 2024/1/19.
//
#include "response_parser.h"


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

static void parse_httpdns_ip_array(cJSON *c_json_array, struct list_head *httpdns_ips) {
    size_t array_size = cJSON_GetArraySize(c_json_array);
    if (array_size == 0) {
        return;
    }
    for (int i = 0; i < array_size; i++) {
        cJSON *resolver_json = cJSON_GetArrayItem(c_json_array, i);
        httpdns_ip_t *httpdns_ip = create_httpdns_ip(resolver_json->valuestring);
        httpdns_list_add(httpdns_ips, httpdns_ip, NULL);
    }
}

httpdns_raw_schedule_result_t *parse_schedule_result(char *body) {
    if (IS_BLANK_SDS(body)) {
        return NULL;
    }
    cJSON *c_json_body = cJSON_Parse(body);
    if (NULL == c_json_body) {
        return NULL;
    }
    bool have_result = false;
    httpdns_raw_schedule_result_t *schedule_result = create_httpdns_raw_schedule_result();
    cJSON *ipv4_resolvers_json = cJSON_GetObjectItem(c_json_body, "service_ip");
    if (NULL != ipv4_resolvers_json) {
        parse_httpdns_ip_array(ipv4_resolvers_json, &schedule_result->service_ip);
        have_result = true;
    }
    cJSON *ipv6_resolvers_json = cJSON_GetObjectItem(c_json_body, "service_ipv6");
    if (NULL != ipv6_resolvers_json) {
        parse_httpdns_ip_array(ipv6_resolvers_json, &schedule_result->service_ipv6);
        have_result = true;
    }
    if (!have_result) {
        free(schedule_result);
        return NULL;
    }
    cJSON_Delete(c_json_body);
    return schedule_result;
}


/*
 * 报文示例：
{
"ipsv6":["240e:960:c00:e:3:0:0:3ef","240e:960:c00:e:3:0:0:3f0"],
"host":"www.aliyun.com",
"client_ip":"47.96.236.37",
"ips":["47.118.227.108","47.118.227.111","47.118.227.112"],
"ttl":60,
"origin_ttl":60
}
 */

static httpdns_raw_single_resolve_result_t *parse_single_resolve_result_from_json(cJSON *c_json_body) {
    if (NULL == c_json_body) {
        return NULL;
    }
    httpdns_raw_single_resolve_result_t *single_resolve_result = create_httpdns_raw_single_resolve_result();
    cJSON *ips_json = cJSON_GetObjectItem(c_json_body, "ips");
    if (NULL != ips_json) {
        parse_httpdns_ip_array(ips_json, &single_resolve_result->ips);
    }
    cJSON *ipsv6_json = cJSON_GetObjectItem(c_json_body, "ipsv6");
    if (NULL != ipsv6_json) {
        parse_httpdns_ip_array(ipsv6_json, &single_resolve_result->ipsv6);
    }
    cJSON *host_json = cJSON_GetObjectItem(c_json_body, "host");
    if (NULL != host_json) {
        single_resolve_result->host = sdsnew(host_json->valuestring);
    }
    cJSON *client_ip_json = cJSON_GetObjectItem(c_json_body, "client_ip");
    if (NULL != client_ip_json) {
        single_resolve_result->client_ip = sdsnew(client_ip_json->valuestring);
    }
    cJSON *ttl_json = cJSON_GetObjectItem(c_json_body, "ttl");
    if (NULL != ttl_json) {
        single_resolve_result->ttl = ttl_json->valueint;
    }
    cJSON *origin_ttl_json = cJSON_GetObjectItem(c_json_body, "origin_ttl");
    if (NULL != origin_ttl_json) {
        single_resolve_result->origin_ttl = origin_ttl_json->valueint;
    }
    cJSON *type_json = cJSON_GetObjectItem(c_json_body, "type");
    if (NULL != type_json) {
        single_resolve_result->type = type_json->valueint;
    }
    return single_resolve_result;
}

httpdns_raw_single_resolve_result_t *parse_single_resolve_result(char *body) {
    if (IS_BLANK_SDS(body)) {
        return NULL;
    }
    cJSON *c_json_body = cJSON_Parse(body);
    if (NULL == c_json_body) {
        return NULL;
    }
    httpdns_raw_single_resolve_result_t *single_resolve_result = parse_single_resolve_result_from_json(c_json_body);
    cJSON_Delete(c_json_body);
    return single_resolve_result;
}

/*
{
	"dns":[{
		"host":"www.aliyun.com",
		"client_ip":"47.96.236.37",
		"ips":["47.118.227.116"],
		"type":1,
		"ttl":26,
		"origin_ttl":60
	},
	{
		"host":"www.taobao.com",
		"client_ip":"47.96.236.37",
		"ips":["240e:f7:a093:101:3:0:0:3e8"],
		"type":28,
		"ttl":60,
		"origin_ttl":60
	}]
}
*/

httpdns_raw_multi_resolve_result_t *parse_multi_resolve_result(char *body) {
    if (IS_BLANK_SDS(body)) {
        return NULL;
    }
    cJSON *c_json_body = cJSON_Parse(body);
    if (NULL == c_json_body) {
        return NULL;
    }
    httpdns_raw_multi_resolve_result_t *mul_resolve_result = create_httpdns_raw_multi_resolve_result();
    cJSON *dns_json = cJSON_GetObjectItem(c_json_body, "dns");
    if (NULL != dns_json) {
        int dns_size = cJSON_GetArraySize(dns_json);
        for (int i = 0; i < dns_size; i++) {
            cJSON *single_result_json = cJSON_GetArrayItem(dns_json, i);
            if (NULL != single_result_json) {
                httpdns_raw_single_resolve_result_t *single_resolve_result = parse_single_resolve_result_from_json(
                        single_result_json);
                httpdns_list_add(&mul_resolve_result->dns, single_resolve_result, NULL);
            }
        }
    }
    cJSON_Delete(c_json_body);
    return mul_resolve_result;
}
