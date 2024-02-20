//
// Created by caogaoshuai on 2024/1/31.
//
#include "httpdns_client_wrapper.h"
#include "httpdns_global_config.h"
#include "log.h"
#include "httpdns_string.h"
#include <pthread.h>
#include "httpdns_memory.h"
#include <string.h>
#include "httpdns_ip.h"

static httpdns_client_t *httpdns_client = NULL;

static httpdns_config_t *httpdns_config = NULL;

static volatile bool is_initialized = false;


int32_t httpdns_client_env_init(const char *account_id, const char *secret_key) {
    if (is_initialized) {
        return HTTPDNS_CLIENT_ALREADY_INITIALIZED;
    }
    init_httpdns_sdk();
    httpdns_config = httpdns_config_new();
    httpdns_config_set_account_id(httpdns_config, account_id);
    httpdns_config_set_secret_key(httpdns_config, secret_key);
    int32_t valid_config_result = httpdns_config_valid(httpdns_config);
    if (HTTPDNS_SUCCESS != valid_config_result) {
        httpdns_client_env_cleanup();
        return HTTPDNS_FAILURE;
    }
    httpdns_client = httpdns_client_new(httpdns_config);
    if (NULL == httpdns_client) {
        httpdns_client_env_cleanup();
        return HTTPDNS_FAILURE;
    }
    is_initialized = true;
    return HTTPDNS_SUCCESS;
}

httpdns_config_t *httpdns_client_get_config() {
    return httpdns_config;
}


void httpdns_client_process_pre_resolve_hosts() {
    get_httpdns_results_for_hosts_async_with_cache(&httpdns_config->pre_resolve_hosts,
                                                   HTTPDNS_QUERY_TYPE_AUTO,
                                                   NULL,
                                                   NULL,
                                                   NULL);
}

int32_t httpdns_client_env_cleanup() {
    if (NULL != httpdns_config) {
        httpdns_config_free(httpdns_config);
    }
    if (NULL != httpdns_client) {
        httpdns_client_free(httpdns_client);
    }
    cleanup_httpdns_sdk();
    return 0;
}

httpdns_resolve_result_t *get_httpdns_result_for_host_sync_with_custom_request(httpdns_resolve_request_t *request) {
    if (!is_initialized) {
        log_info("get_httpdns_result_for_host_sync_with_custom_request failed, httpdns client is not initialized");
        return NULL;
    }
    if (NULL != request && NULL != request->complete_callback_func) {
        log_info("get_httpdns_result_for_host_sync_with_custom_request failed, callback should be NULL");
        return NULL;
    }
    httpdns_resolve_result_t *result = NULL;
    httpdns_client_simple_resolve(httpdns_client,
                                  request,
                                  &result);
    return result;
}

httpdns_resolve_result_t *get_httpdns_result_for_host_sync_with_cache(const char *host,
                                                                      const char *query_type,
                                                                      const char *client_ip) {
    if (!is_initialized) {
        log_info("get_httpdns_result_for_host_sync_with_cache failed, httpdns client is not initialized");
        return NULL;
    }
    httpdns_resolve_request_t *request = httpdns_resolve_request_new(httpdns_config,
                                                                     host,
                                                                     NULL,
                                                                     query_type);
    httpdns_resolve_request_set_client_ip(request, client_ip);
    httpdns_resolve_request_set_using_cache(request, true);
    httpdns_resolve_result_t *result = NULL;
    httpdns_client_simple_resolve(httpdns_client,
                                  request,
                                  &result);
    httpdns_resolve_request_free(request);
    return result;
}

httpdns_resolve_result_t *get_httpdns_result_for_host_sync_without_cache(const char *host,
                                                                         const char *query_type,
                                                                         const char *client_ip) {
    if (!is_initialized) {
        log_info("get_httpdns_result_for_host_sync_with_cache failed, httpdns client is not initialized");
        return NULL;
    }
    httpdns_resolve_request_t *request = httpdns_resolve_request_new(httpdns_config,
                                                                     host,
                                                                     NULL,
                                                                     query_type);
    httpdns_resolve_request_set_client_ip(request, client_ip);
    httpdns_resolve_request_set_using_cache(request, false);
    httpdns_resolve_result_t *result = NULL;
    httpdns_client_simple_resolve(httpdns_client,
                                  request,
                                  &result);
    httpdns_resolve_request_free(request);
    return result;
}


static void *httpdns_single_resolve_routine(void *arg) {
#ifdef __APPLE__
    pthread_setname_np(__func__);
#elif defined(__linux__)
    pthread_setname_np(pthread_self(),__func__);
#endif
    httpdns_resolve_request_t *request = arg;
    httpdns_resolve_result_t *result = NULL;
    httpdns_client_simple_resolve(httpdns_client,
                                  request,
                                  &result);
    if (NULL != result) {
        httpdns_resolve_result_free(result);
    }
    if (NULL != request) {
        httpdns_resolve_request_free(request);
    }
    pthread_exit(NULL);
}

int32_t get_httpdns_result_for_host_async_with_custom_request(httpdns_resolve_request_t *request) {
    if (NULL == request || NULL == request->complete_callback_func) {
        log_info("get_httpdns_result_for_host_async_with_custom_request failed, request or callback is empty");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    pthread_t tid;
    int ret = pthread_create(&tid, NULL, httpdns_single_resolve_routine, request);
    if (0 != ret) {
        log_info("create thread error, ret %d", ret);
        return HTTPDNS_THREAD_CREATE_FAIL_ERROR;
    }
    pthread_detach(tid);
    return HTTPDNS_SUCCESS;
}


static int32_t get_httpdns_result_for_host_async(const char *host,
                                                 const char *query_type,
                                                 const char *client_ip,
                                                 bool using_cache,
                                                 httpdns_complete_callback_func_t cb,
                                                 void *cb_param) {
    if (!is_initialized) {
        log_info("get_httpdns_result_for_host_sync failed, httpdns client is not initialized");
        return HTTPDNS_CLIENT_NOT_INITIALIZE;
    }
    if (IS_BLANK_STRING(host) || IS_BLANK_STRING(query_type) || NULL == cb) {
        log_info("get_httpdns_result_for_host_async failed, host or query_type or cb is empty");
        return HTTPDNS_PARAMETER_ERROR;
    }

    httpdns_resolve_request_t *request = httpdns_resolve_request_new(httpdns_config,
                                                                     host,
                                                                     NULL,
                                                                     query_type);
    httpdns_resolve_request_set_client_ip(request, client_ip);
    httpdns_resolve_request_set_using_cache(request, using_cache);
    httpdns_resolve_request_set_callback(request, cb, cb_param);

    pthread_t tid;
    int ret = pthread_create(&tid, NULL, httpdns_single_resolve_routine, request);
    if (0 != ret) {
        log_info("create thread error, ret %d", ret);
        return HTTPDNS_THREAD_CREATE_FAIL_ERROR;
    }
    pthread_detach(tid);
    return HTTPDNS_SUCCESS;
}

int32_t get_httpdns_result_for_host_async_with_cache(const char *host,
                                                     const char *query_type,
                                                     const char *client_ip,
                                                     httpdns_complete_callback_func_t cb,
                                                     void *cb_param) {
    return get_httpdns_result_for_host_async(host, query_type, client_ip, true, cb, cb_param);
}

int32_t get_httpdns_result_for_host_async_without_cache(const char *host,
                                                        const char *query_type,
                                                        const char *client_ip,
                                                        httpdns_complete_callback_func_t cb,
                                                        void *cb_param) {
    return get_httpdns_result_for_host_async(host, query_type, client_ip, false, cb, cb_param);
}


static int32_t get_httpdns_results_for_hosts(httpdns_list_head_t *hosts,
                                             const char *query_type,
                                             const char *client_ip,
                                             bool using_cache,
                                             httpdns_complete_callback_func_t cb,
                                             void *cb_param,
                                             httpdns_list_head_t *results) {
    if (httpdns_list_is_empty(hosts) || IS_BLANK_STRING(query_type) || NULL == results) {
        log_info("batch get httpdns failed, hosts or query type is NULL");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    httpdns_resolve_task_t *resolve_task = httpdns_resolve_task_new(httpdns_client);
    int host_count = 0;
    char *host_group = NULL;
    httpdns_list_for_each_entry(host_cursor, hosts) {
        if (NULL == host_group) {
            host_group = httpdns_sds_empty();
        }
        if (strlen(host_group) > 0) {
            SDS_CAT(host_group, ",");
        }
        SDS_CAT(host_group, host_cursor->data);
        host_count++;
        if (host_count % MULTI_RESOLVE_SIZE != 0 && !httpdns_list_is_end_node(host_cursor, hosts)) {
            continue;
        }
        httpdns_resolve_request_t *request = httpdns_resolve_request_new(httpdns_client->config,
                                                                         host_group,
                                                                         NULL,
                                                                         query_type);
        if (IS_NOT_BLANK_STRING(client_ip)) {
            httpdns_resolve_request_set_client_ip(request, client_ip);
        }
        httpdns_resolve_request_set_using_cache(request, using_cache);
        httpdns_resolve_request_set_using_multi(request, true);
        httpdns_resolve_request_set_callback(request, cb, cb_param);
        httpdns_resolve_request_set_timeout_ms(request, httpdns_config->timeout_ms);

        httpdns_resolve_task_add_request(resolve_task, request);

        httpdns_sds_t request_str = httpdns_resolve_request_to_string(request);
        log_debug("batch resolve add request %s", request_str);
        httpdns_sds_free(request_str);

        httpdns_resolve_request_free(request);
        httpdns_sds_free(host_group);
        host_group = NULL;
    }

    int32_t ret = httpdns_resolve_task_execute(resolve_task);
    httpdns_list_for_each_entry(resolve_context_cursor, &resolve_task->resolve_contexts) {
        httpdns_resolve_context_t *resolve_context = resolve_context_cursor->data;
        httpdns_list_for_each_entry(resolve_result_cursor, &resolve_context->result) {
            httpdns_list_add(results, resolve_result_cursor->data, to_httpdns_data_clone_func(httpdns_resolve_result_clone));
        }
    }
    httpdns_resolve_task_free(resolve_task);
    return ret;
}


int32_t get_httpdns_results_for_hosts_sync_without_cache(httpdns_list_head_t *hosts,
                                                         const char *query_type,
                                                         httpdns_list_head_t *results,
                                                         const char *client_ip) {
    return get_httpdns_results_for_hosts(hosts, query_type, client_ip, false, NULL, NULL, results);
}

int32_t
get_httpdns_results_for_hosts_sync_with_cache(httpdns_list_head_t *hosts,
                                              const char *query_type,
                                              const char *client_ip,
                                              httpdns_list_head_t *results) {
    return get_httpdns_results_for_hosts(hosts, query_type, client_ip, true, NULL, NULL, results);
}

typedef struct {
    httpdns_list_head_t *hosts;
    const char *query_type;
    const char *client_ip;
    bool using_cache;
    httpdns_complete_callback_func_t cb;
    void *cb_param;
} private_httpdns_multi_resolve_routine_arg_t;

static void *httpdns_multi_resolve_routine(void *arg) {
#ifdef __APPLE__
    pthread_setname_np(__func__);
#elif defined(__linux__)
    pthread_setname_np(pthread_self(),__func__);
#endif
    private_httpdns_multi_resolve_routine_arg_t *batch_routine_arg = arg;

    httpdns_list_new_empty_in_stack(results);
    get_httpdns_results_for_hosts(batch_routine_arg->hosts,
                                  batch_routine_arg->query_type,
                                  batch_routine_arg->client_ip,
                                  batch_routine_arg->using_cache,
                                  batch_routine_arg->cb,
                                  batch_routine_arg->cb_param,
                                  &results);

    httpdns_list_free(&results, to_httpdns_data_free_func(httpdns_resolve_result_free));
    free(arg);
    pthread_exit(NULL);
}


static int32_t get_httpdns_results_for_hosts_async(httpdns_list_head_t *hosts,
                                                   const char *query_type,
                                                   const char *client_ip,
                                                   bool using_cache,
                                                   httpdns_complete_callback_func_t cb,
                                                   void *cb_param) {
    if (!is_initialized) {
        log_info("get_httpdns_results_for_hosts_async failed, httpdns client is not initialized");
        return HTTPDNS_CLIENT_NOT_INITIALIZE;
    }
    if (httpdns_list_is_empty(hosts) || IS_BLANK_STRING(query_type) || NULL == cb) {
        log_info("get_httpdns_results_for_hosts_async failed, hosts or query_type or cb is empty");
        return HTTPDNS_PARAMETER_ERROR;
    }

    HTTPDNS_NEW_OBJECT_IN_HEAP(batch_routine_arg, private_httpdns_multi_resolve_routine_arg_t);
    batch_routine_arg->hosts = hosts;
    batch_routine_arg->query_type = query_type;
    batch_routine_arg->client_ip = client_ip;
    batch_routine_arg->cb = cb;
    batch_routine_arg->using_cache = using_cache;
    batch_routine_arg->cb_param = cb_param;

    pthread_t tid;
    int ret = pthread_create(&tid, NULL, httpdns_multi_resolve_routine, batch_routine_arg);
    if (0 != ret) {
        log_info("create thread error, ret %d", ret);
        return HTTPDNS_THREAD_CREATE_FAIL_ERROR;
    }
    pthread_detach(tid);
    return HTTPDNS_SUCCESS;
}

int32_t
get_httpdns_results_for_hosts_async_with_cache(httpdns_list_head_t *hosts,
                                               const char *query_type,
                                               const char *client_ip,
                                               httpdns_complete_callback_func_t cb,
                                               void *cb_param) {
    return get_httpdns_results_for_hosts_async(hosts, query_type, client_ip, true, cb, cb_param);
}

int32_t get_httpdns_results_for_hosts_async_without_cache(httpdns_list_head_t *hosts,
                                                          const char *query_type,
                                                          const char *client_ip,
                                                          httpdns_complete_callback_func_t cb,
                                                          void *cb_param) {
    return get_httpdns_results_for_hosts_async(hosts, query_type, client_ip, false, cb, cb_param);
}


int32_t select_ip_from_httpdns_result(httpdns_resolve_result_t *result, char *dst_ip_buffer) {
    if (!is_initialized) {
        return HTTPDNS_CLIENT_NOT_INITIALIZE;
    }
    if (NULL == result || NULL == dst_ip_buffer || (httpdns_list_is_empty(&result->ipsv6) && httpdns_list_is_empty(&result->ips))) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    httpdns_list_head_t *ip_list;
    httpdns_net_stack_detector_t *detector = httpdns_client->net_stack_detector;
    net_stack_type_t net_stype = httpdns_net_stack_type_get(detector);
    if (HAVE_IPV4_NET_TYPE(net_stype)) {
        ip_list = &result->ips;
    } else {
        ip_list = &result->ipsv6;
    }
    if (httpdns_list_is_empty(ip_list)) {
        return HTTPDNS_RESULT_IP_EMPTY;
    }
    httpdns_ip_t *httpdns_ip = httpdns_list_get(ip_list, 0);
    httpdns_list_rotate(ip_list);
    strcpy(dst_ip_buffer, httpdns_ip->ip);
    return HTTPDNS_SUCCESS;
}