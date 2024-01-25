//
// Created by cagaoshuai on 2024/1/9.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H

#include "httpdns_scheduler.h"
#include "httpdns_config.h"


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


typedef struct {
    char *host;
    char *account_id;
    char *secret_key;
    char *resolver;
    char *query_type;
    char *client_ip;
    char *sdk_version;
    char *user_agent;
    char *sdns_params;
    bool using_https;
    bool using_sign;
    bool using_multi;
    bool using_cache;
    int32_t timeout_ms;
    char *cache_key;
} httpdns_resolve_request_t;

typedef struct {
    char *host;
    char *client_ip;
    char *extra;
    struct list_head ips;
    struct list_head ipsv6;
    int origin_ttl;
    int ttl;
    struct timeval query_ts;
    void *cache_key;
    bool hit_cache;
} httpdns_resolve_result_t;

typedef void (*http_finish_callback_func_t)(char *response_body, int32_t response_status, int32_t response_rt_ms,
                                            void *user_callback_param);

typedef struct {
    httpdns_resolve_request_t *request;
    struct list_head result;
} httpdns_resolve_context_t;

typedef struct {
    httpdns_resolve_request_t *request;
    void *user_http_finish_callback_param;
    data_free_function_ptr_t callback_param_free_func;
    http_finish_callback_func_t http_finish_callback_func;
} httpdns_resolve_param_t;

void httpdns_resolve_request_append_sdns_params(httpdns_resolve_request_t *request, char *key, char *value);

void httpdns_resolve_request_set_host(httpdns_resolve_request_t *request, char *host);

void httpdns_resolve_request_set_account_id(httpdns_resolve_request_t *request, char *account_id);

void httpdns_resolve_request_set_secret_key(httpdns_resolve_request_t *request, char *secret_key);

void httpdns_resolve_request_set_resolver(httpdns_resolve_request_t *request, char *resolver);

void httpdns_resolve_request_set_query_type(httpdns_resolve_request_t *request, char *query_type);

void httpdns_resolve_request_set_client_ip(httpdns_resolve_request_t *request, char *client_ip);

void httpdns_resolve_request_set_sdk_version(httpdns_resolve_request_t *request, char *sdk_version);

void httpdns_resolve_request_set_user_agent(httpdns_resolve_request_t *request, char *user_agent);

void httpdns_resolve_request_set_cache_key(httpdns_resolve_request_t *request, char *cache_key);

void httpdns_resolve_request_set_timeout_ms(httpdns_resolve_request_t *request, int32_t timeout_ms);

void httpdns_resolve_request_set_using_https(httpdns_resolve_request_t *request, bool using_https);

void httpdns_resolve_request_set_using_sign(httpdns_resolve_request_t *request, bool using_sign);

void httpdns_resolve_request_set_using_multi(httpdns_resolve_request_t *request, bool using_multi);

httpdns_resolve_request_t *
httpdns_resolve_request_create(httpdns_config_t *config, char *host, char *resolver, char *query_type);

httpdns_resolve_request_t *httpdns_resolve_request_clone(httpdns_resolve_request_t *origin_resolve_request);

void httpdns_resolve_request_print(httpdns_resolve_request_t *origin_resolve_request);

int32_t httpdns_resolve_request_valid(httpdns_resolve_request_t *request);

int32_t httpdns_resolver_single_resolve(httpdns_resolve_param_t *resolve_param);

int32_t httpdns_resolver_multi_resolve(struct list_head *resolve_params);

httpdns_resolve_context_t *httpdns_resolve_context_create(httpdns_resolve_request_t *request);

httpdns_resolve_context_t *httpdns_resolve_context_clone(httpdns_resolve_context_t *origin_context);

void httpdns_resolve_context_destroy(httpdns_resolve_context_t *resolve_context);

void httpdns_resolve_request_destroy(httpdns_resolve_request_t *request);

void httpdns_resolve_result_destroy(httpdns_resolve_result_t *result);

httpdns_resolve_result_t *httpdns_resolve_result_clone(httpdns_resolve_result_t *origin_result);

void httpdns_resolve_result_print(httpdns_resolve_result_t *result);

void httpdns_resolve_result_set_cache_key(httpdns_resolve_result_t *result, char* cache_key);

void httpdns_resolve_result_set_hit_cache(httpdns_resolve_result_t *result, bool hit_cache);

void httpdns_resolve_param_destroy(httpdns_resolve_param_t *resolve_param);

httpdns_resolve_param_t *httpdns_resolve_param_create(httpdns_resolve_request_t *request);

httpdns_resolve_param_t *httpdns_resolve_param_clone(httpdns_resolve_param_t *origin_resolve_param);



#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H
