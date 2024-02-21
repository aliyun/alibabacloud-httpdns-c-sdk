//
// Created by caogaoshuai on 2024/1/19.
//
#include "http_response_parser.h"
#include "httpdns_memory.h"
#include <stdio.h>
#include "httpdns_log.h"
#include "httpdns_sds.h"

httpdns_schedule_response_t *httpdns_schedule_response_new() {
    httpdns_new_object_in_heap(schedule_response, httpdns_schedule_response_t);
    httpdns_list_init(&schedule_response->service_ip);
    httpdns_list_init(&schedule_response->service_ipv6);
    return schedule_response;
}

httpdns_sds_t httpdns_schedule_response_to_string(const httpdns_schedule_response_t *response) {
    if (NULL == response) {
        return httpdns_sds_new("httpdns_schedule_response_t()");
    }
    httpdns_sds_t dst_str = httpdns_sds_new("httpdns_schedule_response_t(");
    httpdns_sds_cat_easily(dst_str, "service_ip=");
    httpdns_sds_t list = httpdns_list_to_string(&response->service_ip, NULL);
    httpdns_sds_cat_easily(dst_str, list);
    httpdns_sds_free(list);
    httpdns_sds_cat_easily(dst_str, ",service_ipv6=");
    list = httpdns_list_to_string(&response->service_ipv6, NULL);
    httpdns_sds_cat_easily(dst_str, list);
    httpdns_sds_free(list);
    httpdns_sds_cat_easily(dst_str, ")");
    return dst_str;
}

void httpdns_schedule_response_free(httpdns_schedule_response_t *response) {
    if (NULL == response) {
        return;
    }
    httpdns_list_free(&response->service_ip, httpdns_string_free_func);
    httpdns_list_free(&response->service_ipv6, httpdns_string_free_func);
    free(response);
}

httpdns_single_resolve_response_t *httpdns_single_resolve_response_new() {
    httpdns_new_object_in_heap(single_resolve_response, httpdns_single_resolve_response_t);
    httpdns_list_init(&single_resolve_response->ips);
    httpdns_list_init(&single_resolve_response->ipsv6);
    return single_resolve_response;
}


httpdns_sds_t httpdns_single_resolve_response_to_string(const httpdns_single_resolve_response_t *response) {
    if (NULL == response) {
        return httpdns_sds_new("httpdns_single_resolve_response_t()");
    }
    httpdns_sds_t dst_str = httpdns_sds_new("httpdns_single_resolve_response_t(host=");
    httpdns_sds_cat_easily(dst_str, response->host);
    httpdns_sds_cat_easily(dst_str, ",ips=");
    httpdns_sds_t list = httpdns_list_to_string(&response->ips, NULL);
    httpdns_sds_cat_easily(dst_str, list);
    httpdns_sds_free(list);

    httpdns_sds_cat_easily(dst_str, ",ipsv6=");
    list = httpdns_list_to_string(&response->ipsv6, NULL);
    httpdns_sds_cat_easily(dst_str, list);
    httpdns_sds_free(list);

    httpdns_sds_cat_easily(dst_str, ",ttl=");
    httpdns_sds_cat_int(dst_str, response->ttl)

    httpdns_sds_cat_easily(dst_str, ",origin_ttl=");
    httpdns_sds_cat_int(dst_str, response->origin_ttl)

    httpdns_sds_cat_easily(dst_str, ",extra=");
    httpdns_sds_cat_easily(dst_str, response->extra);

    httpdns_sds_cat_easily(dst_str, ",client_ip=");
    httpdns_sds_cat_easily(dst_str, response->client_ip);

    httpdns_sds_cat_easily(dst_str, ")");
    return dst_str;
}

void httpdns_single_resolve_response_free(httpdns_single_resolve_response_t *response) {
    if (NULL == response) {
        return;
    }
    if (NULL != response->host) {
        httpdns_sds_free(response->host);
    }
    if (NULL != response->extra) {
        httpdns_sds_free(response->extra);
    }
    if (NULL != response->client_ip) {
        httpdns_sds_free(response->client_ip);
    }
    httpdns_list_free(&response->ips, httpdns_string_free_func);
    httpdns_list_free(&response->ipsv6, httpdns_string_free_func);
    free(response);
}

httpdns_multi_resolve_response_t *httpdns_multi_resolve_response_new() {
    httpdns_new_object_in_heap(multi_resolve_result, httpdns_multi_resolve_response_t);
    httpdns_list_init(&multi_resolve_result->dns);
    return multi_resolve_result;
}

httpdns_sds_t httpdns_multi_resolve_response_to_string(const httpdns_multi_resolve_response_t *response) {
    if (NULL == response) {
        return httpdns_sds_new("httpdns_multi_resolve_response_t()");
    }
    httpdns_sds_t dst_str = httpdns_sds_new("httpdns_multi_resolve_response_t(");
    httpdns_sds_t list = httpdns_list_to_string(&response->dns, to_httpdns_data_to_string_func(httpdns_single_resolve_response_to_string));
    httpdns_sds_cat_easily(dst_str, list);
    httpdns_sds_cat_easily(dst_str, ")");
    httpdns_sds_free(list);
    return dst_str;
}

void httpdns_multi_resolve_response_free(httpdns_multi_resolve_response_t *response) {
    if (NULL == response) {
        return;
    }
    httpdns_list_free(&response->dns, to_httpdns_data_free_func(httpdns_single_resolve_response_free));
    free(response);
}

static void parse_ip_array(cJSON *c_json_array, httpdns_list_head_t *ips) {
    size_t array_size = cJSON_GetArraySize(c_json_array);
    if (array_size == 0) {
        return;
    }
    for (int i = 0; i < array_size; i++) {
        cJSON *ip_json = cJSON_GetArrayItem(c_json_array, i);
        httpdns_list_add(ips, ip_json->valuestring, httpdns_string_clone_func);
    }
}

httpdns_schedule_response_t *httpdns_response_parse_schedule(const char *body) {
    if (httpdns_string_is_blank(body)) {
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

static char *decode_html(char *str) {
    char *src = str;
    char *dst = httpdns_sds_empty();
    size_t length = strlen(str);
    size_t index = 0;

    while (index < length) {
        if (src[index] == '&' && index + 4 < length && strncmp(&src[index], "&amp;", 5) == 0) {
            httpdns_sds_cat_easily(dst, "&");
            index += 5;
        } else if (src[index] == '&' && index + 3 < length && strncmp(&src[index], "&lt;", 4) == 0) {
            httpdns_sds_cat_easily(dst, "<");
            index += 4;
        } else if (src[index] == '&' && index + 3 < length && strncmp(&src[index], "&gt;", 4) == 0) {
            httpdns_sds_cat_easily(dst, ">");
            index += 4;
        } else if (src[index] == '&' && index + 5 < length && strncmp(&src[index], "&quot;", 6) == 0) {
            httpdns_sds_cat_easily(dst, "\"");
            index += 6;
        } else if (src[index] == '&' && index + 5 < length && strncmp(&src[index], "&apos;", 6) == 0) {
            httpdns_sds_cat_easily(dst, "\'");
            index += 6;
        } else {
            httpdns_sds_cat_char_easily(dst, src[index++]);
        }
    }
    return dst;
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
        single_resolve_result->host = httpdns_sds_new(host_json->valuestring);
    }
    cJSON *client_ip_json = cJSON_GetObjectItem(c_json_body, "client_ip");
    if (NULL != client_ip_json) {
        single_resolve_result->client_ip = httpdns_sds_new(client_ip_json->valuestring);
    }
    cJSON *ttl_json = cJSON_GetObjectItem(c_json_body, "ttl");
    if (NULL != ttl_json) {
        single_resolve_result->ttl = ttl_json->valueint;
    }
    cJSON *origin_ttl_json = cJSON_GetObjectItem(c_json_body, "origin_ttl");
    if (NULL != origin_ttl_json) {
        single_resolve_result->origin_ttl = origin_ttl_json->valueint;
    }
    cJSON *extra_json = cJSON_GetObjectItem(c_json_body, "extra");
    if (NULL != extra_json) {
        single_resolve_result->extra = decode_html(extra_json->valuestring);
    }
    // 多域名解析接口
    cJSON *type_json = cJSON_GetObjectItem(c_json_body, "type");
    if (NULL != type_json) {
        int32_t type = type_json->valueint;
        if (type == HTTPDNS_RESOLVE_TYPE_AAAA) {
            httpdns_list_dup(&single_resolve_result->ipsv6, &single_resolve_result->ips, httpdns_string_clone_func);
            httpdns_list_free(&single_resolve_result->ips, httpdns_string_free_func);
        }
    }
    return single_resolve_result;
}

httpdns_single_resolve_response_t *httpdns_response_parse_single_resolve(const char *body) {
    if (httpdns_string_is_blank(body)) {
        httpdns_log_info("parse single resolve failed, body is empty");
        return NULL;
    }
    cJSON *c_json_body = cJSON_Parse(body);
    if (NULL == c_json_body) {
        httpdns_log_info("parse single resolve failed, body may be not json");
        return NULL;
    }
    httpdns_single_resolve_response_t *single_resolve_result = parse_single_resolve_result_from_json(c_json_body);
    if (httpdns_list_is_empty(&single_resolve_result->ips) && httpdns_list_is_empty(&single_resolve_result->ipsv6)) {
        httpdns_log_info("parse single resolve is empty, body is %s", body);
    }
    cJSON_Delete(c_json_body);
    return single_resolve_result;
}

httpdns_multi_resolve_response_t *httpdns_response_parse_multi_resolve(const char *body) {
    if (httpdns_string_is_blank(body)) {
        httpdns_log_info("parse multi resolve failed, body is empty");
        return NULL;
    }
    cJSON *c_json_body = cJSON_Parse(body);
    if (NULL == c_json_body) {
        httpdns_log_info("parse multi resolve failed, body may be not json");
        return NULL;
    }
    httpdns_multi_resolve_response_t *mul_resolve_result = httpdns_multi_resolve_response_new();
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
        httpdns_list_shuffle(&mul_resolve_result->dns);
    } else {
        httpdns_log_info("parse multi resolve failed, body is %s", body);
    }
    cJSON_Delete(c_json_body);
    return mul_resolve_result;
}
