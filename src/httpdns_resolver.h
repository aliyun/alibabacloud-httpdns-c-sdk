//
// 把HTTPDNS解析的相关参数转化为HTTPDNS请求，并通过回调透出HTTPDNS响应结果
//
// Created by caogaoshuai on 2024/1/9.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "httpdns_http.h"
#include "httpdns_client_config.h"
#include "httpdns_resolve_request.h"
#include "httpdns_resolve_result.h"
#include "httpdns_scheduler.h"

#define  HTTPDNS_API_D                   "/d"
#define  HTTPDNS_API_SIGN_D              "/sign_d"
#define  HTTPDNS_API_RESOLVE             "/resolve"
#define  HTTPDNS_API_SIGN_RESOLVE        "/sign_resolve"

#define  HTTPDNS_QUERY_TYPE_A        "4"
#define  HTTPDNS_QUERY_TYPE_AAAA     "6"
#define  HTTPDNS_QUERY_TYPE_BOTH   "4,6"
#define  HTTPDNS_QUERY_TYPE_AUTO   "AUTO"

#define httpdns_is_query_type_a(type) \
    (NULL != type && strcmp(HTTPDNS_QUERY_TYPE_A, type) == 0)

#define httpdns_is_query_type_aaaa(type) \
    (NULL != type && strcmp(HTTPDNS_QUERY_TYPE_AAAA, type) == 0)

#define httpdns_is_query_type_both(type) \
    (NULL != type && strcmp(HTTPDNS_QUERY_TYPE_BOTH, type) == 0)

#define httpdns_is_query_type_auto(type) \
    (NULL != type && strcmp(HTTPDNS_QUERY_TYPE_AUTO, type) == 0)


typedef void (*httpdns_http_complete_callback_func_t)(httpdns_http_context_t *httpdns_context,
                                              void *user_callback_param);

typedef struct {
    httpdns_resolve_request_t *request;
    httpdns_list_head_t result;
} httpdns_resolve_context_t;

typedef struct {
    httpdns_resolve_request_t *request;
    void *user_http_complete_callback_param;
    httpdns_data_free_func_t callback_param_free_func;
    httpdns_http_complete_callback_func_t http_complete_callback_func;
} httpdns_resolve_param_t;

int32_t httpdns_resolver_single_resolve(httpdns_resolve_param_t *resolve_param);

int32_t httpdns_resolver_multi_resolve(httpdns_list_head_t *resolve_params);

/**
 * must free using httpdns_resolve_context_free
 */
httpdns_resolve_context_t *httpdns_resolve_context_new(const httpdns_resolve_request_t *request);

void httpdns_resolve_context_free(httpdns_resolve_context_t *resolve_context);

void httpdns_resolve_param_free(httpdns_resolve_param_t *resolve_param);

/**
 * must free using httpdns_resolve_param_free
 */
httpdns_resolve_param_t *httpdns_resolve_param_new(httpdns_resolve_request_t *request);


#ifdef __cplusplus
}
#endif


#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H
