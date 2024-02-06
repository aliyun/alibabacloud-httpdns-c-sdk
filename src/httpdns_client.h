//
// HTTPDNS组件集成，实现HTTPDNS解析功能，组件包括
// 网络环境检测、解析器、缓存、调度器
//
// Created by caogaoshuai on 2024/1/22.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_CLIENT_H
#define HTTPDNS_C_SDK_HTTPDNS_CLIENT_H

#include "httpdns_scheduler.h"
#include "httpdns_cache.h"
#include "httpdns_resolver.h"
#include "httpdns_resolve_result.h"
#include "httpdns_resolve_request.h"
#include "httpdns_resolver.h"
#include "httpdns_cache.h"

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
    httpdns_scheduler_t *scheduler;
    int32_t retry_times;
    bool is_completed;
} on_http_finish_callback_param_t;

/**
 * must free using httpdns_client_free
 */
httpdns_client_t *httpdns_client_new(httpdns_config_t *config);

void httpdns_client_free(httpdns_client_t *client);

int32_t httpdns_client_simple_resolve(httpdns_client_t *httpdns_client,
                                      httpdns_resolve_request_t *request,
                                        // must free using httpdns_resolve_result_free
                                      httpdns_resolve_result_t **result);

/**
 * must free using httpdns_resolve_task_free
 */
httpdns_resolve_task_t *httpdns_resolve_task_new(httpdns_client_t *httpdns_client);


int32_t httpdns_resolve_task_add_request(httpdns_resolve_task_t *task, const httpdns_resolve_request_t *request);


int32_t httpdns_resolve_task_execute(httpdns_resolve_task_t *task);


void httpdns_resolve_task_free(httpdns_resolve_task_t *resolve_task);

#endif //HTTPDNS_C_SDK_HTTPDNS_CLIENT_H
