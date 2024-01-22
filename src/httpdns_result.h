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
    char *ip;
    int32_t rt;
} httpdns_ip_t;


void print_httpdns_ip(httpdns_ip_t *httpdns_ip);


httpdns_ip_t *create_httpdns_ip(char *ip);

void destroy_httpdns_ip(httpdns_ip_t *ip);

httpdns_ip_t *clone_httpdns_ip(httpdns_ip_t *ip);

void destroy_httpdns_resolve_result(httpdns_resolve_result_t *result);

void print_httpdns_resolve_result(httpdns_resolve_result_t *result);

httpdns_resolve_result_t *clone_httpdns_resolve_result(httpdns_resolve_result_t *origin_result);

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESULT_H
