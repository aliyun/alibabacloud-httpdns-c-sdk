//
// Created by cagaoshuai on 2024/1/10.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESULT_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESULT_H
#include <stdio.h>
#include <stdlib.h>
#include "../libs/list.h"
#include "../libs/dict.h"


typedef struct _httpdns_generic_result {
    void* result;  // 泛型结果，可以指向任何类型的数据
    char* error_message;  // 错误信息，如果函数调用成功，该字段可以为 NULL
    int32_t error_code;
} httpdns_generic_result_t;

#define CAST_RESULT_AS(type, httpdns_generic_result_t) \
    ((type*)((httpdns_generic_result_t).result))

typedef struct _httpdns_resolve_result {
    char* host;
    char* client_ip;
    struct list_head ips;
    struct list_head ipv6s;
    dict   extra;
    int64_t query_timestamp;
    int ttl;
} httpdns_resolve_result_t;





#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESULT_H
