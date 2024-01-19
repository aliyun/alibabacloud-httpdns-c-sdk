//
// Created by cagaoshuai on 2024/1/9.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H

#include "httpdns_scheduler.h"
#include "httpdns_config.h"
#include "httpdns_cache.h"
#include "httpdns_result.h"

#define  HTTPDNS_API_D        "/d"
#define  HTTPDNS_API_SIGN_D   "/sign_d"

typedef struct {
    httpdns_scheduler_t *scheduler;
    net_stack_detector_t *net_stack_detector;
    httpdns_config_t *config;
    httpdns_cache_table_t *cache;
} httpdns_resolver_t;

typedef struct {
    httpdns_resolver_t *resolver;
    struct list_head requests;
    struct list_head results;
} httpdns_resolve_task_t;

typedef enum {
    TYPE_A,
    TYPE_AAAA,
    TYPE_BOTH,
    TYPE_AUTO
} resolve_type_t;

typedef struct {
    char *host;
    resolve_type_t dns_type;
    char *sdns_params;
    char *cache_key;
    char *timeout_ms;
    char *client_ip;
    bool hit_cache;
} httpdns_resolve_request_t;


httpdns_resolve_request_t *create_httpdns_resolve_request(char *host, resolve_type_t dns_type, char *cache_key);

void httpdns_resolve_request_append_sdns_params(httpdns_resolve_request_t *request, char *key, char *value);

void httpdns_resolve_request_set_cache_key(httpdns_resolve_request_t *request, char *cache_key);

void destroy_httpdns_resolve_request(httpdns_resolve_request_t *request);

httpdns_resolve_request_t *clone_httpdns_resolve_request(httpdns_resolve_request_t *request);


httpdns_resolve_task_t *create_httpdns_resolve_task(httpdns_resolver_t *resolver);

void httpdns_resolve_task_add_request(httpdns_resolve_task_t *task, httpdns_resolve_request_t *request);

void httpdns_resolve_task_add_result(httpdns_resolve_task_t *task, httpdns_resolve_result_t *result);

void destroy_httpdns_resolve_task(httpdns_resolve_task_t *task);

/**
 * @description create httpdns resolver
 * @param config httpdns config
 * @return:  resolver
 */
httpdns_resolver_t *create_httpdns_resolver(httpdns_config_t *config);

/**
 * @description resolve domain
 * @param resolver resolver
 * @param host domain to be resolve
 * @param dns_type target DNS record type
 * @return:  httpdns_generic_result_t
 */
int32_t resolve(httpdns_resolve_task_t *task);


/**
 * destroy resolver, this will free all memory allocated by this config
 * @param config
 */
void destroy_httpdns_resolver(httpdns_resolver_t *resolver);

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H
