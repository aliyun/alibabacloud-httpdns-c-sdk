//
// Created by cagaoshuai on 2024/1/19.
//
#include "httpdns_response.h"
#include "httpdns_memory.h"
#include <stdio.h>
#include "log.h"

httpdns_schedule_response_t *httpdns_schedule_response_new() {
    HTTPDNS_NEW_OBJECT_IN_HEAP(schedule_response, httpdns_schedule_response_t);
    httpdns_list_init(&schedule_response->service_ip);
    httpdns_list_init(&schedule_response->service_ipv6);
    return schedule_response;
}

sds httpdns_schedule_response_to_string(httpdns_schedule_response_t *response) {
    if (NULL == response) {
        return sdsnew("httpdns_schedule_response_t()");
    }
    sds dst_str = sdsnew("httpdns_schedule_response_t(");
    SDS_CAT(dst_str, "service_ip=");
    sds list = httpdns_list_to_string(&response->service_ip, NULL);
    SDS_CAT(dst_str, list);
    sdsfree(list);
    SDS_CAT(dst_str, ",service_ipv6=");
    list = httpdns_list_to_string(&response->service_ipv6, NULL);
    SDS_CAT(dst_str, list);
    sdsfree(list);
    SDS_CAT(dst_str, ")");
    return dst_str;
}

void httpdns_schedule_response_free(httpdns_schedule_response_t *response) {
    if (NULL == response) {
        return;
    }
    httpdns_list_free(&response->service_ip, STRING_FREE_FUNC);
    httpdns_list_free(&response->service_ipv6, STRING_FREE_FUNC);
    free(response);
}

httpdns_single_resolve_response_t *httpdns_single_resolve_response_new() {
    HTTPDNS_NEW_OBJECT_IN_HEAP(single_resolve_response, httpdns_single_resolve_response_t);
    httpdns_list_init(&single_resolve_response->ips);
    httpdns_list_init(&single_resolve_response->ipsv6);
    return single_resolve_response;
}


sds httpdns_single_resolve_response_to_string(httpdns_single_resolve_response_t *response) {
    if (NULL == response) {
        return sdsnew("httpdns_single_resolve_response_t()");
    }
    sds dst_str = sdsnew("httpdns_single_resolve_response_t(host=");
    SDS_CAT(dst_str, response->host);
    SDS_CAT(dst_str, ",ips=");
    sds list = httpdns_list_to_string(&response->ips, NULL);
    SDS_CAT(dst_str, list);
    sdsfree(list);

    SDS_CAT(dst_str, ",ipsv6=");
    list = httpdns_list_to_string(&response->ipsv6, NULL);
    SDS_CAT(dst_str, list);
    sdsfree(list);

    SDS_CAT(dst_str, ",ttl=");
    SDS_CAT_INT(dst_str, response->ttl)

    SDS_CAT(dst_str, ",origin_ttl=");
    SDS_CAT_INT(dst_str, response->origin_ttl)

    SDS_CAT(dst_str, ",extra=");
    SDS_CAT(dst_str, response->extra);

    SDS_CAT(dst_str, ",client_ip=%s");
    SDS_CAT(dst_str, response->client_ip);

    SDS_CAT(dst_str, ")");
    return dst_str;
}

void httpdns_single_resolve_response_free(httpdns_single_resolve_response_t *response) {
    if (NULL == response) {
        return;
    }
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

httpdns_multi_resolve_response_t *httpdns_multi_resolve_response_new() {
    HTTPDNS_NEW_OBJECT_IN_HEAP(multi_resolve_result, httpdns_multi_resolve_response_t);
    httpdns_list_init(&multi_resolve_result->dns);
    return multi_resolve_result;
}

sds httpdns_multi_resolve_response_to_string(httpdns_multi_resolve_response_t *response) {
    if (NULL == response) {
        return sdsnew("httpdns_multi_resolve_response_t()");
    }
    sds dst_str = sdsnew("httpdns_multi_resolve_response_t(");
    sds list = httpdns_list_to_string(&response->dns, DATA_TO_STRING_FUNC(httpdns_single_resolve_response_to_string));
    SDS_CAT(dst_str, list);
    SDS_CAT(dst_str, ")");
    return dst_str;
}

void httpdns_multi_resolve_response_free(httpdns_multi_resolve_response_t *response) {
    if (NULL == response) {
        return;
    }
    httpdns_list_free(&response->dns, DATA_FREE_FUNC(httpdns_single_resolve_response_free));
    free(response);
}

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

httpdns_schedule_response_t *httpdns_response_parse_schedule(char *body) {
    if (IS_BLANK_STRING(body)) {
        return NULL;
    }
    cJSON *c_json_body = cJSON_Parse(body);
    if (NULL == c_json_body) {
        return NULL;
    }
    httpdns_schedule_response_t *schedule_result = httpdns_schedule_response_new();
    cJSON *ipv4_resolvers_json = cJSON_GetObjectItem(c_json_body, "service_ip");
    if (NULL != ipv4_resolvers_json) {
        parse_ip_array(ipv4_resolvers_json, &schedule_result->service_ip);
        httpdns_list_shuffle(&schedule_result->service_ip);
    }
    cJSON *ipv6_resolvers_json = cJSON_GetObjectItem(c_json_body, "service_ipv6");
    if (NULL != ipv6_resolvers_json) {
        parse_ip_array(ipv6_resolvers_json, &schedule_result->service_ipv6);
        httpdns_list_shuffle(&schedule_result->service_ipv6);
    }
    cJSON_Delete(c_json_body);
    return schedule_result;
}

static httpdns_single_resolve_response_t *parse_single_resolve_result_from_json(cJSON *c_json_body) {
    if (NULL == c_json_body) {
        return NULL;
    }
    httpdns_single_resolve_response_t *single_resolve_result = httpdns_single_resolve_response_new();
    cJSON *ips_json = cJSON_GetObjectItem(c_json_body, "ips");
    if (NULL != ips_json) {
        parse_ip_array(ips_json, &single_resolve_result->ips);
        httpdns_list_shuffle(&single_resolve_result->ips);
    }
    cJSON *ipsv6_json = cJSON_GetObjectItem(c_json_body, "ipsv6");
    if (NULL != ipsv6_json) {
        parse_ip_array(ipsv6_json, &single_resolve_result->ipsv6);
        httpdns_list_shuffle(&single_resolve_result->ipsv6);
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
    // 多域名解析接口
    cJSON *type_json = cJSON_GetObjectItem(c_json_body, "type");
    if (NULL != type_json) {
        int32_t type = type_json->valueint;
        if (type == RESOLVE_TYPE_AAAA) {
            httpdns_list_dup(&single_resolve_result->ipsv6, &single_resolve_result->ips, STRING_CLONE_FUNC);
            httpdns_list_free(&single_resolve_result->ips, STRING_FREE_FUNC);
        }
    }
    return single_resolve_result;
}

httpdns_single_resolve_response_t *httpdns_response_parse_single_resolve(char *body) {
    if (IS_BLANK_STRING(body)) {
        log_info("parse single resolve failed, body is empty");
        return NULL;
    }
    cJSON *c_json_body = cJSON_Parse(body);
    if (NULL == c_json_body) {
        log_info("parse single resolve failed, body may be not json");
        return NULL;
    }
    httpdns_single_resolve_response_t *single_resolve_result = parse_single_resolve_result_from_json(c_json_body);
    if (IS_EMPTY_LIST(&single_resolve_result->ips) && IS_EMPTY_LIST(&single_resolve_result->ipsv6)) {
        log_info("parse single resolve is empty, body is %s", body);
    }
    cJSON_Delete(c_json_body);
    return single_resolve_result;
}

httpdns_multi_resolve_response_t *httpdns_response_parse_multi_resolve(char *body) {
    if (IS_BLANK_STRING(body)) {
        log_info("parse multi resolve failed, body is empty");
        return NULL;
    }
    cJSON *c_json_body = cJSON_Parse(body);
    if (NULL == c_json_body) {
        log_info("parse multi resolve failed, body may be not json");
        return NULL;
    }
    httpdns_multi_resolve_response_t *mul_resolve_result = httpdns_multi_resolve_response_new();
    cJSON *dns_json = cJSON_GetObjectItem(c_json_body, "results");
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
        httpdns_list_shuffle(&mul_resolve_result->dns);
    } else {
        log_info("parse multi resolve failed, body is %s", body);
    }
    cJSON_Delete(c_json_body);
    return mul_resolve_result;
}
