//
//负责HTTPDNS服务端响应报文解析
//
// Created by caogaoshuai on 2024/1/19.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_HTTP_RESPONSE_PARSER_H
#define HTTPDNS_C_SDK_HTTPDNS_HTTP_RESPONSE_PARSER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "httpdns_list.h"

#define HTTPDNS_RESOLVE_TYPE_A 1
#define HTTPDNS_RESOLVE_TYPE_AAAA 28

typedef struct {
    httpdns_list_head_t service_ip;
    httpdns_list_head_t service_ipv6;
} httpdns_schedule_response_t;

typedef struct {
    char *host;
    httpdns_list_head_t ips;
    httpdns_list_head_t ipsv6;
    int32_t ttl;
    int32_t origin_ttl;
    char *extra;
    char *client_ip;
} httpdns_single_resolve_response_t;

typedef struct {
    httpdns_list_head_t dns;
} httpdns_multi_resolve_response_t;

/**
 * must free using httpdns_schedule_response_free
 */
httpdns_schedule_response_t *httpdns_schedule_response_new();

/**
 * must free using httpdns_sds_free
 */
httpdns_sds_t httpdns_schedule_response_to_string(const httpdns_schedule_response_t *response);

void httpdns_schedule_response_free(httpdns_schedule_response_t *response);

/**
 * must free using httpdns_single_resolve_response_free
 */
httpdns_single_resolve_response_t *httpdns_single_resolve_response_new();

/**
 * must free using httpdns_sds_free
 */
httpdns_sds_t httpdns_single_resolve_response_to_string(const httpdns_single_resolve_response_t *response);

void httpdns_single_resolve_response_free(httpdns_single_resolve_response_t *response);

/**
 * must free using httpdns_multi_resolve_response_free
 */
httpdns_multi_resolve_response_t *httpdns_multi_resolve_response_new();

/**
 * must free using httpdns_sds_free
 */
httpdns_sds_t httpdns_multi_resolve_response_to_string(const httpdns_multi_resolve_response_t *response);

void httpdns_multi_resolve_response_free(httpdns_multi_resolve_response_t *response);

/**
 * must free using httpdns_schedule_response_free
 */
httpdns_schedule_response_t *httpdns_response_parse_schedule(const char *body);

/**
 * must free using httpdns_single_resolve_response_free
 */
httpdns_single_resolve_response_t *httpdns_response_parse_single_resolve(const char *body);

/**
 * must free using httpdns_multi_resolve_response_free
 */
httpdns_multi_resolve_response_t *httpdns_response_parse_multi_resolve(const char *body);

#ifdef __cplusplus
}
#endif

#endif //HTTPDNS_C_SDK_HTTPDNS_HTTP_RESPONSE_PARSER_H
