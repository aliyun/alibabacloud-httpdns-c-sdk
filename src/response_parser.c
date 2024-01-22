//
// Created by cagaoshuai on 2024/1/19.
//
#include "response_parser.h"
#include "httpdns_memory.h"

httpdns_schedule_response_t *httpdns_schedule_response_create() {
    HTTPDNS_NEW_OBJECT_IN_HEAP(schedule_response, httpdns_schedule_response_t);
    httpdns_list_init(&schedule_response->service_ip);
    httpdns_list_init(&schedule_response->service_ipv6);
    return schedule_response;
}

void httpdns_schedule_response_print(httpdns_schedule_response_t *response) {
    if (NULL != response) {
        printf("httpdns_schedule_response_t(");
        printf("service_ip=");
        httpdns_list_print(&response->service_ip, STRING_PRINT_FUNC);
        printf(",service_ipv6=");
        httpdns_list_print(&response->service_ipv6, STRING_PRINT_FUNC);
        printf(")");
    }
}

void httpdns_schedule_response_destroy(httpdns_schedule_response_t *response) {
    if (NULL != response) {
        httpdns_list_free(&response->service_ip, STRING_FREE_FUNC);
        httpdns_list_free(&response->service_ipv6, STRING_FREE_FUNC);
        free(response);
    }
}

httpdns_single_resolve_response_t *httpdns_single_resolve_response_create() {
    HTTPDNS_NEW_OBJECT_IN_HEAP(single_resolve_response, httpdns_single_resolve_response_t);
    httpdns_list_init(&single_resolve_response->ips);
    httpdns_list_init(&single_resolve_response->ipsv6);
    return single_resolve_response;
}


void httpdns_single_resolve_response_print(httpdns_single_resolve_response_t *response) {
    if (NULL != response) {
        printf("httpdns_single_resolve_response_t(host=%s,", response->host);
        printf(",ips=");
        httpdns_list_print(&response->ips, STRING_PRINT_FUNC);
        printf(",ipsv6=");
        httpdns_list_print(&response->ipsv6, STRING_PRINT_FUNC);
        printf(",ttl=%d", response->ttl);
        printf(",origin_ttl=%d", response->origin_ttl);
        printf(",extra=%s", response->extra);
        printf(",client_ip=%s", response->client_ip);
        printf(",type=%d)", response->type);
    }
}

void httpdns_single_resolve_response_destroy(httpdns_single_resolve_response_t *response) {
    if (NULL != response) {
        if (NULL != response->host) {
            sdsfree(response->host);
        }
        if (NULL != response->extra) {
            sdsfree(response->extra);
        }
        if (NULL != response->client_ip) {
            sdsfree(response->client_ip);
        }
        httpdns_list_free(&response->ips, STRING_FREE_FUNC);
        httpdns_list_free(&response->ipsv6, STRING_FREE_FUNC);
        free(response);
    }
}

httpdns_multi_resolve_response_t *httpdns_multi_resolve_response_create() {
    HTTPDNS_NEW_OBJECT_IN_HEAP(multi_resolve_result, httpdns_multi_resolve_response_t);
    httpdns_list_init(&multi_resolve_result->dns);
    return multi_resolve_result;
}

void httpdns_multi_resolve_response_print(httpdns_multi_resolve_response_t *response) {
    if (NULL != response) {
        printf("httpdns_multi_resolve_response_t(");
        httpdns_list_print(&response->dns, DATA_PRINT_FUNC(httpdns_single_resolve_response_print));
        printf(")");
    }
}

void httpdns_multi_resolve_response_destroy(httpdns_multi_resolve_response_t *response) {
    if (NULL != response) {
        httpdns_list_free(&response->dns, DATA_FREE_FUNC(httpdns_single_resolve_response_destroy));
        free(response);
    }
}


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

static void parse_ip_array(cJSON *c_json_array, struct list_head *ips) {
    size_t array_size = cJSON_GetArraySize(c_json_array);
    if (array_size == 0) {
        return;
    }
    for (int i = 0; i < array_size; i++) {
        cJSON *ip_json = cJSON_GetArrayItem(c_json_array, i);
        httpdns_list_add(ips, ip_json->valuestring, STRING_CLONE_FUNC);
    }
}

httpdns_schedule_response_t *parse_schedule_response(char *body) {
    if (IS_BLANK_STRING(body)) {
        return NULL;
    }
    cJSON *c_json_body = cJSON_Parse(body);
    if (NULL == c_json_body) {
        return NULL;
    }
    httpdns_schedule_response_t *schedule_result = httpdns_schedule_response_create();
    cJSON *ipv4_resolvers_json = cJSON_GetObjectItem(c_json_body, "service_ip");
    if (NULL != ipv4_resolvers_json) {
        parse_ip_array(ipv4_resolvers_json, &schedule_result->service_ip);
    }
    cJSON *ipv6_resolvers_json = cJSON_GetObjectItem(c_json_body, "service_ipv6");
    if (NULL != ipv6_resolvers_json) {
        parse_ip_array(ipv6_resolvers_json, &schedule_result->service_ipv6);
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

static httpdns_single_resolve_response_t *parse_single_resolve_result_from_json(cJSON *c_json_body) {
    if (NULL == c_json_body) {
        return NULL;
    }
    httpdns_single_resolve_response_t *single_resolve_result = httpdns_single_resolve_response_create();
    cJSON *ips_json = cJSON_GetObjectItem(c_json_body, "ips");
    if (NULL != ips_json) {
        parse_ip_array(ips_json, &single_resolve_result->ips);
    }
    cJSON *ipsv6_json = cJSON_GetObjectItem(c_json_body, "ipsv6");
    if (NULL != ipsv6_json) {
        parse_ip_array(ipsv6_json, &single_resolve_result->ipsv6);
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

httpdns_single_resolve_response_t *parse_single_resolve_response(char *body) {
    if (IS_BLANK_STRING(body)) {
        return NULL;
    }
    cJSON *c_json_body = cJSON_Parse(body);
    if (NULL == c_json_body) {
        return NULL;
    }
    httpdns_single_resolve_response_t *single_resolve_result = parse_single_resolve_result_from_json(c_json_body);
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

httpdns_multi_resolve_response_t *parse_multi_resolve_response(char *body) {
    if (IS_BLANK_STRING(body)) {
        return NULL;
    }
    cJSON *c_json_body = cJSON_Parse(body);
    if (NULL == c_json_body) {
        return NULL;
    }
    httpdns_multi_resolve_response_t *mul_resolve_result = httpdns_multi_resolve_response_create();
    cJSON *dns_json = cJSON_GetObjectItem(c_json_body, "dns");
    if (NULL != dns_json) {
        int dns_size = cJSON_GetArraySize(dns_json);
        for (int i = 0; i < dns_size; i++) {
            cJSON *single_result_json = cJSON_GetArrayItem(dns_json, i);
            if (NULL != single_result_json) {
                httpdns_single_resolve_response_t *single_resolve_result = parse_single_resolve_result_from_json(
                        single_result_json);
                httpdns_list_add(&mul_resolve_result->dns, single_resolve_result, NULL);
            }
        }
    }
    cJSON_Delete(c_json_body);
    return mul_resolve_result;
}
