//
// Created by cagaoshuai on 2024/1/19.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_RESPONSE_H
#define HTTPDNS_C_SDK_HTTPDNS_RESPONSE_H

#include "cJSON.h"
#include "sds.h"
#include "httpdns_list.h"

#define RESOLVE_TYPE_A 1
#define RESOLVE_TYPE_AAAA 28

typedef struct {
    struct list_head service_ip;
    struct list_head service_ipv6;
} httpdns_schedule_response_t;

typedef struct {
    char *host;
    struct list_head ips;
    struct list_head ipsv6;
    int32_t ttl;
    int32_t origin_ttl;
    char *extra;
    char *client_ip;
} httpdns_single_resolve_response_t;

typedef struct {
    struct list_head dns;
} httpdns_multi_resolve_response_t;

httpdns_schedule_response_t *httpdns_schedule_response_new();

sds httpdns_schedule_response_to_string(httpdns_schedule_response_t *response);

void httpdns_schedule_response_free(httpdns_schedule_response_t *response);

httpdns_single_resolve_response_t *httpdns_single_resolve_response_new();

sds httpdns_single_resolve_response_to_string(httpdns_single_resolve_response_t *response);

void httpdns_single_resolve_response_free(httpdns_single_resolve_response_t *response);

httpdns_multi_resolve_response_t *httpdns_multi_resolve_response_new();

sds httpdns_multi_resolve_response_to_string(httpdns_multi_resolve_response_t *response);

void httpdns_multi_resolve_response_free(httpdns_multi_resolve_response_t *response);

httpdns_schedule_response_t *httpdns_response_parse_schedule(char *body);

httpdns_single_resolve_response_t *httpdns_response_parse_single_resolve(char *body);

httpdns_multi_resolve_response_t *httpdns_response_parse_multi_resolve(char *body);

#endif //HTTPDNS_C_SDK_HTTPDNS_RESPONSE_H
