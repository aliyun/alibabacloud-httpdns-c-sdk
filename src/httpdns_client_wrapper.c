//
// Created by cagaoshuai on 2024/1/31.
//
#include "httpdns_client_wrapper.h"
#include "httpdns_global_config.h"
#include "log.h"
#include "httpdns_string.h"
#include <pthread.h>
#include "httpdns_memory.h"
#include <string.h>

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

httpdns_config_t *get_httpdns_client_config() {
    return httpdns_config;
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


httpdns_resolve_result_t *get_httpdns_result_for_host_sync_with_cache(const char *host,
                                                                      const char *query_type,
                                                                      const char *client_ip) {
    if (!is_initialized) {
        log_info("get_httpdns_result_for_host_sync_with_cache failed, httpdns client is not initialized");
        return NULL;
    }
    httpdns_resolve_result_t *result = NULL;
    httpdns_client_simple_resolve(httpdns_client,
                                  host,
                                  query_type,
                                  client_ip,
                                  true,
                                  &result,
                                  NULL,
                                  NULL);
    return result;
}

httpdns_resolve_result_t *get_httpdns_result_for_host_sync_without_cache(const char *host,
                                                                         const char *query_type,
                                                                         const char *client_ip) {
    if (!is_initialized) {
        log_info("get_httpdns_result_for_host_sync_with_cache failed, httpdns client is not initialized");
        return NULL;
    }
    httpdns_resolve_result_t *result = NULL;
    httpdns_client_simple_resolve(httpdns_client,
                                  host,
                                  query_type,
                                  client_ip,
                                  false,
                                  &result,
                                  NULL,
                                  NULL);
    return result;
}


static int32_t batch_get_httpdns_result_for_hosts(struct list_head *hosts,
                                                  const char *query_type,
                                                  const char *client_ip,
                                                  bool using_cache,
                                                  httpdns_complete_callback_func_t cb,
                                                  void *cb_param,
                                                  struct list_head *results) {
    if (IS_EMPTY_LIST(hosts) || IS_BLANK_STRING(query_type) || NULL == results) {
        log_info("batch get httpdns failed, hosts or query type is NULL");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    httpdns_resolve_task_t *resolve_task = httpdns_resolve_task_new(httpdns_client);
    int host_count = 0;
    char *host_group = NULL;
    httpdns_list_for_each_entry(host_cursor, hosts) {
        if (NULL == host_group) {
            host_group = sdsempty();
        }
        if (strlen(host_group) > 0) {
            SDS_CAT(host_group, ",");
        }
        SDS_CAT(host_group, host_cursor->data);
        host_count++;
        if (host_count % MULTI_RESOLVE_SIZE != 0 && !httpdns_list_is_end(host_cursor, hosts)) {
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

        sds request_str = httpdns_resolve_request_to_string(request);
        log_debug("batch resolve add request %s", request_str);
        sdsfree(request_str);

        httpdns_resolve_request_free(request);
        sdsfree(host_group);
        host_group = NULL;
    }

    int32_t ret = httpdns_resolve_task_execute(resolve_task);
    httpdns_list_for_each_entry(resolve_context_cursor, &resolve_task->resolve_contexts) {
        httpdns_resolve_context_t *resolve_context = resolve_context_cursor->data;
        httpdns_list_for_each_entry(resolve_result_cursor, &resolve_context->result) {
            httpdns_list_add(results, resolve_result_cursor->data, DATA_CLONE_FUNC(httpdns_resolve_result_clone));
        }
    }
    httpdns_resolve_task_free(resolve_task);
    return ret;
}


int32_t batch_get_httpdns_result_for_hosts_sync_without_cache(struct list_head *hosts,
                                                              const char *query_type,
                                                              struct list_head *results,
                                                              const char *client_ip) {
    return batch_get_httpdns_result_for_hosts(hosts, query_type, client_ip, false, NULL, NULL, results);
}

int32_t
batch_get_httpdns_result_for_hosts_sync_with_cache(struct list_head *hosts,
                                                   const char *query_type,
                                                   const char *client_ip,
                                                   struct list_head *results) {
    return batch_get_httpdns_result_for_hosts(hosts, query_type, client_ip, true, NULL, NULL, results);
}


typedef struct {
    const char *host;
    const char *query_type;
    const char *client_ip;
    bool using_cache;
    httpdns_complete_callback_func_t cb;
    void *cb_param;
} private_httpdns_routine_arg_t;

static void *httpdns_routine(void *arg) {
#ifdef __APPLE__
    pthread_setname_np(__func__);
#elif defined(__linux__)
    pthread_setname_np(pthread_self(),__func__);
#endif
    private_httpdns_routine_arg_t *routine_arg = arg;


    httpdns_resolve_result_t *result = NULL;
    httpdns_client_simple_resolve(httpdns_client,
                                  routine_arg->host,
                                  routine_arg->query_type,
                                  routine_arg->client_ip,
                                  true,
                                  &result,
                                  routine_arg->cb,
                                  routine_arg->cb_param);
    if (NULL != result) {
        httpdns_resolve_result_free(result);
    }
    free(arg);
    pthread_exit(NULL);
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

    HTTPDNS_NEW_OBJECT_IN_HEAP(routine_arg, private_httpdns_routine_arg_t);
    routine_arg->host = host;
    routine_arg->query_type = query_type;
    routine_arg->client_ip = client_ip;
    routine_arg->cb = cb;
    routine_arg->using_cache = using_cache;
    routine_arg->cb_param = cb_param;

    pthread_t tid;
    int ret = pthread_create(&tid, NULL, httpdns_routine, routine_arg);
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


typedef struct {
    struct list_head *hosts;
    const char *query_type;
    const char *client_ip;
    bool using_cache;
    httpdns_complete_callback_func_t cb;
    void *cb_param;
} private_httpdns_batch_routine_arg_t;

static void *httpdns_batch_routine(void *arg) {
#ifdef __APPLE__
    pthread_setname_np(__func__);
#elif defined(__linux__)
    pthread_setname_np(pthread_self(),__func__);
#endif
    private_httpdns_batch_routine_arg_t *batch_routine_arg = arg;

    NEW_EMPTY_LIST_IN_STACK(results);
    batch_get_httpdns_result_for_hosts(batch_routine_arg->hosts,
                                       batch_routine_arg->query_type,
                                       batch_routine_arg->client_ip,
                                       batch_routine_arg->using_cache,
                                       batch_routine_arg->cb,
                                       batch_routine_arg->cb_param,
                                       &results);

    httpdns_list_free(&results, DATA_FREE_FUNC(httpdns_resolve_result_free));
    free(arg);
    pthread_exit(NULL);
}


static int32_t batch_get_httpdns_result_for_hosts_async(struct list_head *hosts,
                                                        const char *query_type,
                                                        const char *client_ip,
                                                        bool using_cache,
                                                        httpdns_complete_callback_func_t cb,
                                                        void *cb_param) {
    if (!is_initialized) {
        log_info("batch_get_httpdns_result_for_hosts_async failed, httpdns client is not initialized");
        return HTTPDNS_CLIENT_NOT_INITIALIZE;
    }
    if (IS_EMPTY_LIST(hosts) || IS_BLANK_STRING(query_type) || NULL == cb) {
        log_info("batch_get_httpdns_result_for_hosts_async failed, hosts or query_type or cb is empty");
        return HTTPDNS_PARAMETER_ERROR;
    }

    HTTPDNS_NEW_OBJECT_IN_HEAP(batch_routine_arg, private_httpdns_batch_routine_arg_t);
    batch_routine_arg->hosts = hosts;
    batch_routine_arg->query_type = query_type;
    batch_routine_arg->client_ip = client_ip;
    batch_routine_arg->cb = cb;
    batch_routine_arg->using_cache = using_cache;
    batch_routine_arg->cb_param = cb_param;

    pthread_t tid;
    int ret = pthread_create(&tid, NULL, httpdns_batch_routine, batch_routine_arg);
    if (0 != ret) {
        log_info("create thread error, ret %d", ret);
        return HTTPDNS_THREAD_CREATE_FAIL_ERROR;
    }
    pthread_detach(tid);
    return HTTPDNS_SUCCESS;
}

int32_t
batch_get_httpdns_result_for_hosts_async_with_cache(struct list_head *hosts,
                                                    const char *query_type,
                                                    const char *client_ip,
                                                    httpdns_complete_callback_func_t cb,
                                                    void *cb_param) {
    return batch_get_httpdns_result_for_hosts_async(hosts, query_type, client_ip, true, cb, cb_param);
}

int32_t batch_get_httpdns_result_for_hosts_async_without_cache(struct list_head *hosts,
                                                               const char *query_type,
                                                               const char *client_ip,
                                                               httpdns_complete_callback_func_t cb,
                                                               void *cb_param) {
    return batch_get_httpdns_result_for_hosts_async(hosts, query_type, client_ip, false, cb, cb_param);
}
