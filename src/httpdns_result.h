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
    struct list_head ipv6s;
    int origin_ttl;
    int ttl;
    struct timespec query_ts;
} httpdns_resolve_result_t;

typedef struct {
    char *ip;
    int32_t connect_time_ms;
} httpdns_ip_t;

void destroy_httpdns_ip(httpdns_ip_t* ip);

void destroy_httpdns_resolve_result(httpdns_resolve_result_t * result);

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESULT_H
