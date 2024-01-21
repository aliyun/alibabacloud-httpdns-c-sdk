//
// Created by cagaoshuai on 2024/1/10.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESULT_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESULT_H

#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "dict.h"
#include <time.h>
#include <stdbool.h>

#define DEFAULT_IP_RT        0

typedef enum {
    A = 1,
    AAAA = 28
} dns_type_t;

typedef struct _httpdns_generic_result {
    void *result;  // 泛型结果，可以指向任何类型的数据
    char *error_message;  // 错误信息，如果函数调用成功，该字段可以为 NULL
    int32_t error_code;
} httpdns_generic_result_t;

#define CAST_RESULT_AS(type, httpdns_generic_result_t) \
    ((type*)((httpdns_generic_result_t).result))

typedef struct {
    char *host;
    char *client_ip;
    char *extra;
    struct list_head ips;
    struct list_head ipsv6;
    int origin_ttl;
    int ttl;
    struct timespec query_ts;
    char *cache_key;
    bool hit_cache;
} httpdns_resolve_result_t;

typedef struct {
    struct list_head service_ip;
    struct list_head service_ipv6;
} httpdns_raw_schedule_result_t;

typedef struct {
    char *host;
    struct list_head ips;
    struct list_head ipsv6;
    int32_t ttl;
    int32_t origin_ttl;
    char *extra;
    char *client_ip;
    dns_type_t type;
} httpdns_raw_single_resolve_result_t;

typedef struct {
    struct list_head dns;
} httpdns_raw_multi_resolve_result_t;

typedef struct {
    char *ip;
    int32_t rt;
} httpdns_ip_t;

void print_httpdns_ip(httpdns_ip_t *httpdns_ip);

void print_httpdns_raw_schedule_result(httpdns_raw_schedule_result_t *result);

void print_httpdns_raw_single_resolve_result(httpdns_raw_single_resolve_result_t *result);

void print_httpdns_raw_multi_resolve_result(httpdns_raw_multi_resolve_result_t *result);

httpdns_raw_schedule_result_t *create_httpdns_raw_schedule_result();

void destroy_httpdns_raw_schedule_result(httpdns_raw_schedule_result_t *result);

httpdns_raw_single_resolve_result_t *create_httpdns_raw_single_resolve_result();

void destroy_httpdns_raw_single_resolve_result(httpdns_raw_single_resolve_result_t *result);

httpdns_raw_multi_resolve_result_t *create_httpdns_raw_multi_resolve_result();

void destroy_httpdns_raw_multi_resolve_result(httpdns_raw_multi_resolve_result_t *result);

httpdns_ip_t *create_httpdns_ip(char *ip);

void destroy_httpdns_ip(httpdns_ip_t *ip);

httpdns_ip_t *clone_httpdns_ip(httpdns_ip_t *ip);

void destroy_httpdns_resolve_result(httpdns_resolve_result_t *result);

void print_httpdns_resolve_result(httpdns_resolve_result_t *result);

httpdns_resolve_result_t *clone_httpdns_resolve_result(httpdns_resolve_result_t *origin_result);

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESULT_H
