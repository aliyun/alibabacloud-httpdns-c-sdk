//
// Created by cagaoshuai on 2024/1/22.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_CLIENT_H
#define HTTPDNS_C_SDK_HTTPDNS_CLIENT_H

#include "httpdns_scheduler.h"
#include "httpdns_cache.h"
#include "httpdns_resolver.h"

typedef struct {
    httpdns_scheduler_t *scheduler;
    httpdns_net_stack_detector_t *net_stack_detector;
    httpdns_config_t *config;
    httpdns_cache_table_t *cache;
} httpdns_client_t;

typedef struct {
    httpdns_client_t *httpdns_client;
    struct list_head resolve_contexts;
} httpdns_resolve_task_t;

typedef struct {
    httpdns_resolve_context_t *resolve_context;
    httpdns_cache_table_t *cache_table;
    httpdns_scheduler_t * scheduler;
} on_http_finish_callback_param_t;


httpdns_client_t *httpdns_client_create(httpdns_config_t *config);

void httpdns_client_destroy(httpdns_client_t * client);


int32_t httpdns_client_simple_resolve(httpdns_client_t *httpdns_client,
                                      char *host,
                                      char* query_type,
                                      char *client_ip,
                                      httpdns_resolve_result_t **result);


httpdns_resolve_task_t *httpdns_resolve_task_create(httpdns_client_t *httpdns_client);


int32_t httpdns_resolve_task_add_request(httpdns_resolve_task_t *task, httpdns_resolve_request_t *request);


int32_t httpdns_resolve_task_execute(httpdns_resolve_task_t *task);


void httpdns_resolve_task_destroy(httpdns_resolve_task_t *task);

#endif //HTTPDNS_C_SDK_HTTPDNS_CLIENT_H
