//
// 把HTTPDNS解析的相关参数转化为HTTPDNS请求，并通过回调透出HTTPDNS响应结果
//
// Created by caogaoshuai on 2024/1/9.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H

#include "httpdns_scheduler.h"
#include "httpdns_client_config.h"
#include "httpdns_resolve_request.h"
#include "httpdns_resolve_result.h"

#define  HTTPDNS_API_D                   "/d"
#define  HTTPDNS_API_SIGN_D              "/sign_d"
#define  HTTPDNS_API_RESOLVE             "/resolve"
#define  HTTPDNS_API_SIGN_RESOLVE        "/sign_resolve"

#define  HTTPDNS_QUERY_TYPE_A        "4"
#define  HTTPDNS_QUERY_TYPE_AAAA     "6"
#define  HTTPDNS_QUERY_TYPE_BOTH   "4,6"
#define  HTTPDNS_QUERY_TYPE_AUTO   "AUTO"

#define IS_TYPE_A(type) \
    (NULL != type && strcmp(HTTPDNS_QUERY_TYPE_A, type) == 0)

#define IS_TYPE_AAAA(type) \
    (NULL != type && strcmp(HTTPDNS_QUERY_TYPE_AAAA, type) == 0)

#define IS_TYPE_BOTH(type) \
    (NULL != type && strcmp(HTTPDNS_QUERY_TYPE_BOTH, type) == 0)

#define IS_TYPE_AUTO(type) \
    (NULL != type && strcmp(HTTPDNS_QUERY_TYPE_AUTO, type) == 0)


typedef void (*http_complete_callback_func_t)(char *response_body, int32_t response_status, int32_t response_rt_ms,
                                              void *user_callback_param);

typedef struct {
    httpdns_resolve_request_t *request;
    struct list_head result;
} httpdns_resolve_context_t;

typedef struct {
    httpdns_resolve_request_t *request;
    void *user_http_complete_callback_param;
    data_free_function_ptr_t callback_param_free_func;
    http_complete_callback_func_t http_complete_callback_func;
} httpdns_resolve_param_t;

int32_t httpdns_resolver_single_resolve(httpdns_resolve_param_t *resolve_param);

int32_t httpdns_resolver_multi_resolve(struct list_head *resolve_params);

httpdns_resolve_context_t *httpdns_resolve_context_new(const httpdns_resolve_request_t *request);

void httpdns_resolve_context_free(httpdns_resolve_context_t *resolve_context);

void httpdns_resolve_param_free(httpdns_resolve_param_t *resolve_param);

httpdns_resolve_param_t *httpdns_resolve_param_new(httpdns_resolve_request_t *request);

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H
