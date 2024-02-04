//
// Created by caogaoshuai on 2024/1/22.
//
#include "httpdns_client.h"
#include "httpdns_memory.h"
#include "httpdns_http.h"
#include "http_response_parser.h"
#include "httpdns_time.h"
#include "httpdns_ip.h"
#include "httpdns_string.h"
#include "log.h"


httpdns_client_t *httpdns_client_new(httpdns_config_t *config) {
    if (httpdns_config_valid(config) != HTTPDNS_SUCCESS) {
        log_error("create httpdns client failed, config is invalid");
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(httpdns_client, httpdns_client_t);
    httpdns_client->config = config;
    httpdns_client->net_stack_detector = httpdns_net_stack_detector_new();
    httpdns_net_stack_detector_set_probe_domain(httpdns_client->net_stack_detector, config->probe_domain);
    httpdns_client->scheduler = httpdns_scheduler_new(config);
    httpdns_scheduler_set_net_stack_detector(httpdns_client->scheduler, httpdns_client->net_stack_detector);
    httpdns_client->cache = httpdns_cache_table_new();
    httpdns_scheduler_refresh(httpdns_client->scheduler);
    return httpdns_client;
}

void httpdns_client_free(httpdns_client_t *client) {
    if (NULL == client) {
        return;
    }
    if (NULL != client->net_stack_detector) {
        httpdns_net_stack_detector_free(client->net_stack_detector);
    }
    if (NULL != client->scheduler) {
        httpdns_scheduler_free(client->scheduler);
    }
    if (NULL != client->cache) {
        httpdns_cache_table_free(client->cache);
    }
    free(client);
}


httpdns_resolve_task_t *httpdns_resolve_task_new(httpdns_client_t *httpdns_client) {
    if (NULL == httpdns_client) {
        log_info("create httpdns resolve task failed, httpdns client is NULL");
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(resolve_task, httpdns_resolve_task_t);
    resolve_task->httpdns_client = httpdns_client;
    httpdns_list_init(&resolve_task->resolve_contexts);
    return resolve_task;
}


int32_t httpdns_resolve_task_add_request(httpdns_resolve_task_t *task, const httpdns_resolve_request_t *request) {
    if (NULL == task || NULL == request) {
        log_info("add resolve request into task failed, task or request is NULL");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (IS_BLANK_STRING(request->account_id)) {
        log_info("add resolve request into task failed, account_id is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    httpdns_resolve_context_t *resolve_context = httpdns_resolve_context_new(request);
    httpdns_list_add(&task->resolve_contexts, resolve_context, NULL);
    return HTTPDNS_SUCCESS;
}


void httpdns_resolve_task_free(httpdns_resolve_task_t *resolve_task) {
    if (NULL == resolve_task) {
        return;
    }
    httpdns_list_free(&resolve_task->resolve_contexts, DATA_FREE_FUNC(httpdns_resolve_context_free));
    free(resolve_task);
}

static httpdns_resolve_result_t *single_resolve_response_to_result(httpdns_single_resolve_response_t *response) {
    if (NULL == response) {
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(result, httpdns_resolve_result_t);
    if (NULL != response->host) {
        result->host = sdsnew(response->host);
        result->cache_key = sdsnew(response->host);
    }
    if (NULL != response->client_ip) {
        result->client_ip = sdsnew(response->client_ip);
    }
    if (NULL != response->extra) {
        result->extra = sdsnew(response->extra);
    }
    httpdns_list_dup(&result->ips, &response->ips, DATA_CLONE_FUNC(httpdns_ip_new));
    httpdns_list_dup(&result->ipsv6, &response->ipsv6, DATA_CLONE_FUNC(httpdns_ip_new));
    result->ttl = response->ttl;
    result->origin_ttl = response->origin_ttl;
    result->query_ts = httpdns_time_now();
    result->hit_cache = false;
    return result;
}

static void on_http_complete_callback_func(httpdns_http_context_t *http_context,
                                           void *user_callback_param) {
    sds http_context_str = httpdns_http_context_to_string(http_context);
    log_debug("on_http_complete_callback_func %s", http_context_str);
    sdsfree(http_context_str);

    if (NULL == user_callback_param) {
        log_debug("user callback param is NULL, skip");
        return;
    }

    on_http_finish_callback_param_t *param = (on_http_finish_callback_param_t *) user_callback_param;
    httpdns_resolve_context_t *resolve_context = param->resolve_context;
    httpdns_resolve_request_t *resolve_request = resolve_context->request;
    httpdns_scheduler_t *scheduler = param->scheduler;
    param->retry_times = param->retry_times - 1;
    if (http_context->response_status != HTTP_STATUS_OK) {
        sds http_context_str = httpdns_http_context_to_string(http_context);
        log_info("http response exception, httpdns_conxtext %s", http_context_str);
        sdsfree(http_context_str);
        char *resolver = resolve_context->request->resolver;
        httpdns_scheduler_update(param->scheduler, resolver, MAX_HTTP_REQUEST_TIMEOUT_MS);
        return;
    }
    // 结果收集
    NEW_EMPTY_LIST_IN_STACK(httpdns_resolve_results);
    if (resolve_request->using_multi) {
        httpdns_multi_resolve_response_t *response = httpdns_response_parse_multi_resolve(http_context->response_body);
        httpdns_list_dup(&httpdns_resolve_results, &response->dns, DATA_CLONE_FUNC(single_resolve_response_to_result));
        httpdns_multi_resolve_response_free(response);
        httpdns_list_for_each_entry(resolve_result_cursor, &httpdns_resolve_results) {
            httpdns_resolve_result_t *result = resolve_result_cursor->data;
            httpdns_resolve_result_set_cache_key(result, result->host);
        }
    } else {
        httpdns_single_resolve_response_t *response = httpdns_response_parse_single_resolve(http_context->response_body);
        httpdns_resolve_result_t *result = single_resolve_response_to_result(response);
        httpdns_resolve_result_set_cache_key(result, resolve_request->cache_key);
        httpdns_list_add(&httpdns_resolve_results, result, NULL);
        httpdns_single_resolve_response_free(response);
    }
    // 结果合并
    NEW_EMPTY_LIST_IN_STACK(httpdns_merged_resolve_results);
    httpdns_resolve_results_merge(&httpdns_resolve_results, &httpdns_merged_resolve_results);
    httpdns_list_free(&httpdns_resolve_results, DATA_FREE_FUNC(httpdns_resolve_result_free));
    // 结果回调
    httpdns_list_for_each_entry(result_cursor, &httpdns_merged_resolve_results) {
        httpdns_resolve_result_t *resolve_result = result_cursor->data;
        //添加结果
        httpdns_list_add(&param->resolve_context->result, resolve_result,
                         DATA_CLONE_FUNC(httpdns_resolve_result_clone));
        //更新缓存
        if (NULL != param->cache_table) {
            httpdns_cache_table_update(param->cache_table, resolve_result);
        }
        //更新调度器
        httpdns_scheduler_update(scheduler, resolve_request->resolver, http_context->response_rt_ms);
        //用户自定义回调
        if (NULL != resolve_request->complete_callback_func) {
            resolve_request->complete_callback_func(resolve_result, resolve_request->user_callback_param);
        }
    }
    httpdns_list_free(&httpdns_merged_resolve_results, DATA_FREE_FUNC(httpdns_resolve_result_free));
    param->is_completed = true;
}


static char *determine_miss_query_type(httpdns_cache_entry_t *cache_entry, char *expected_query_type) {
    //  未命中缓存，直接查询
    if (NULL == cache_entry) {
        return expected_query_type;
    }
    // 部分命中缓存，查找缺失结果
    if (IS_TYPE_BOTH(expected_query_type) && IS_EMPTY_LIST(&cache_entry->ips)) {
        return HTTPDNS_QUERY_TYPE_A;
    }
    if (IS_TYPE_BOTH(expected_query_type) && IS_EMPTY_LIST(&cache_entry->ipsv6)) {
        return HTTPDNS_QUERY_TYPE_AAAA;
    }
    // 直接使用缓存
    return NULL;
}

static void on_http_finish_callback_param_free(on_http_finish_callback_param_t *param) {
    if (NULL != param) {
        free(param);
    }
}

static char *unwrap_auto_query_type(httpdns_net_stack_detector_t *detector) {
    net_stack_type_t type = httpdns_net_stack_type_get(detector);
    switch (type) {
        case IPV4_ONLY:
            return HTTPDNS_QUERY_TYPE_A;
        case IPV6_ONLY:
            return HTTPDNS_QUERY_TYPE_AAAA;
        default:
            return HTTPDNS_QUERY_TYPE_BOTH;
    }
}

int32_t httpdns_resolve_task_execute(httpdns_resolve_task_t *task) {
    if (NULL == task || NULL == task->httpdns_client || IS_EMPTY_LIST(&task->resolve_contexts)) {
        log_info("httpdns execute resolve task failed, task or client or contexts is empty");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    httpdns_cache_table_t *cache_table = task->httpdns_client->cache;
    httpdns_scheduler_t *scheduler = task->httpdns_client->scheduler;
    httpdns_net_stack_detector_t *net_stack_detector = task->httpdns_client->net_stack_detector;
    httpdns_config_t *config = task->httpdns_client->config;
    NEW_EMPTY_LIST_IN_STACK(resolve_params);
    httpdns_list_for_each_entry(resolve_context_cursor, &task->resolve_contexts) {
        httpdns_resolve_context_t *resolve_context = resolve_context_cursor->data;
        httpdns_resolve_request_t *request = resolve_context->request;
        if (NULL == request->resolver) {
            log_debug("fill resolver into request");
            request->resolver = httpdns_scheduler_get(scheduler);
        }
        if (IS_TYPE_AUTO(request->query_type)) {
            log_debug("unwrap auto query type");
            httpdns_resolve_request_set_query_type(request, unwrap_auto_query_type(net_stack_detector));
        }
        char *query_dns_type = request->query_type;
        if (!request->using_multi && request->using_cache) {
            httpdns_resolve_result_t *cache_entry = httpdns_cache_table_get(cache_table,
                                                                            request->cache_key,
                                                                            request->query_type);
            query_dns_type = determine_miss_query_type(cache_entry, request->query_type);
            if (NULL == query_dns_type) {
                httpdns_cache_entry_rotate(cache_entry);
                httpdns_resolve_result_t *result = httpdns_resolve_result_clone(cache_entry);
                httpdns_resolve_result_set_hit_cache(result, true);
                httpdns_list_add(&resolve_context->result, result, NULL);
                if (NULL != request->complete_callback_func) {
                    request->complete_callback_func(result, request->user_callback_param);
                }
                log_debug("hit cache, cache_key %s, query_type %s, skip", request->cache_key, request->query_type);
                continue;
            }
        }
        httpdns_resolve_param_t *resolve_param = httpdns_resolve_param_new(resolve_context->request);
        httpdns_resolve_request_set_query_type(resolve_param->request, query_dns_type);

        HTTPDNS_NEW_OBJECT_IN_HEAP(on_http_finish_callback_param, on_http_finish_callback_param_t);
        on_http_finish_callback_param->cache_table = cache_table;
        on_http_finish_callback_param->resolve_context = resolve_context;
        on_http_finish_callback_param->scheduler = scheduler;
        on_http_finish_callback_param->retry_times = config->retry_times;
        on_http_finish_callback_param->is_completed = false;

        resolve_param->user_http_complete_callback_param = on_http_finish_callback_param;
        resolve_param->callback_param_free_func = DATA_FREE_FUNC(on_http_finish_callback_param_free);
        resolve_param->http_complete_callback_func = on_http_complete_callback_func;


        httpdns_list_add(&resolve_params, resolve_param, NULL);
    }
    // 多轮执行
    int32_t query_resolve_param_size;
    do {
        query_resolve_param_size = 0;
        NEW_EMPTY_LIST_IN_STACK(query_reoslve_params);
        httpdns_list_for_each_entry(resolve_param_curosr, &resolve_params) {
            httpdns_resolve_param_t *resolve_param = resolve_param_curosr->data;
            on_http_finish_callback_param_t *on_http_finish_callback_param = resolve_param->user_http_complete_callback_param;
            if (on_http_finish_callback_param->is_completed) {
                continue;
            }
            if (on_http_finish_callback_param->retry_times < 0) {
                continue;
            }
            // 重试时更换服务IP
            httpdns_resolve_param_t *new_resolve_param = httpdns_resolve_param_new(resolve_param->request);
            sds new_resolver=httpdns_scheduler_get(scheduler);
            httpdns_resolve_request_set_resolver(new_resolve_param->request, new_resolver);
            sdsfree(new_resolver);

            // 这里不释放参数，只进行参数传递
            new_resolve_param->callback_param_free_func = NULL;
            new_resolve_param->http_complete_callback_func = resolve_param->http_complete_callback_func;
            new_resolve_param->user_http_complete_callback_param = resolve_param->user_http_complete_callback_param;

            query_resolve_param_size++;

            httpdns_list_add(&query_reoslve_params, new_resolve_param, NULL);
        }
        httpdns_resolver_multi_resolve(&query_reoslve_params);
        httpdns_list_free(&query_reoslve_params, DATA_FREE_FUNC(httpdns_resolve_param_free));
    } while (query_resolve_param_size > 0);


    httpdns_list_free(&resolve_params, DATA_FREE_FUNC(httpdns_resolve_param_free));
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_client_simple_resolve(httpdns_client_t *httpdns_client,
                                      const char *host,
                                      const char *query_type,
                                      const char *client_ip,
                                      bool using_cache,
                                      httpdns_resolve_result_t **result,
                                      httpdns_complete_callback_func_t callback,
                                      void *user_callback_param) {
    if (NULL == httpdns_client
        || NULL == httpdns_client->config
        || IS_BLANK_STRING(host)
        || IS_BLANK_STRING(query_type)) {
        log_debug("simple resolve failed, client or config or host or query type is empty");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    httpdns_resolve_request_t *request = httpdns_resolve_request_new(httpdns_client->config,
                                                                     host,
                                                                     NULL,
                                                                     query_type);
    if (IS_NOT_BLANK_STRING(client_ip)) {
        httpdns_resolve_request_set_client_ip(request, client_ip);
    }
    httpdns_resolve_request_set_callback(request, callback, user_callback_param);
    httpdns_resolve_request_set_using_cache(request, using_cache);

    httpdns_resolve_task_t *resolve_task = httpdns_resolve_task_new(httpdns_client);
    httpdns_resolve_task_add_request(resolve_task, request);
    int32_t ret = httpdns_resolve_task_execute(resolve_task);
    if (HTTPDNS_SUCCESS != ret) {
        return ret;
    }
    if (httpdns_list_size(&resolve_task->resolve_contexts) > 0) {
        httpdns_resolve_context_t *resolve_context = httpdns_list_get(&resolve_task->resolve_contexts, 0);
        if (httpdns_list_size(&resolve_context->result) > 0) {
            *result = httpdns_resolve_result_clone(httpdns_list_get(&resolve_context->result, 0));
        }
    }
    httpdns_resolve_request_free(request);
    httpdns_resolve_task_free(resolve_task);
    if (NULL != *result) {
        return HTTPDNS_SUCCESS;
    }
    return HTTPDNS_FAILURE;
}

