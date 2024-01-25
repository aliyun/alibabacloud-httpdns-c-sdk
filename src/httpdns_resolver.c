//
// Created by cagaoshuai on 2024/1/18.
//

#include "httpdns_resolver.h"
#include "httpdns_sign.h"
#include "httpdns_response.h"
#include "httpdns_memory.h"
#include "httpdns_time.h"
#include "httpdns_ip.h"

int32_t httpdns_resolve_request_valid(httpdns_resolve_request_t *request) {
    if (NULL == request) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (IS_BLANK_STRING(request->host)
        || IS_BLANK_STRING(request->account_id)
        || IS_BLANK_STRING(request->resolver)
        || IS_BLANK_STRING(request->query_type)
        || IS_BLANK_STRING(request->user_agent)
        || IS_BLANK_STRING(request->sdk_version)) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    // 批量解析不允许自定义缓存key
    if (request->using_multi && IS_NOT_BLANK_STRING(request->cache_key)) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    // FIXME 如果是批量解析，则域名数不能超过5个
    // FIXME 批量解析接口不能自定义cache_key
    if (request->timeout_ms <= 0) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_resolver_single_resolve(httpdns_resolve_param_t *resolve_param) {
    if (NULL == resolve_param || NULL == resolve_param->request) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    httpdns_resolve_request_t *request = resolve_param->request;
    if (httpdns_resolve_request_valid(request) != HTTPDNS_SUCCESS) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    NEW_EMPTY_LIST_IN_STACK(resolve_params);
    httpdns_list_add(&resolve_params, resolve_param, NULL);
    int32_t ret = httpdns_resolver_multi_resolve(&resolve_params);
    httpdns_list_free(&resolve_params, NULL);
    return ret;
}

int32_t httpdns_resolver_multi_resolve(struct list_head *resolve_params) {
    if (NULL == resolve_params) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    size_t resolve_params_size = httpdns_list_size(resolve_params);
    if (resolve_params_size <= 0) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    size_t http_context_size = 0;
    NEW_EMPTY_LIST_IN_STACK(http_contexts);
    for (int i = 0; i < resolve_params_size; i++) {
        httpdns_resolve_param_t *resolve_param = httpdns_list_get(resolve_params, i);
        httpdns_resolve_request_t *request = resolve_param->request;
        if (HTTPDNS_SUCCESS != httpdns_resolve_request_valid(request)) {
            continue;
        }
        const char *http_scheme = request->using_https ? HTTPS_SCHEME : HTTP_SCHEME;
        const bool using_sign = (request->using_sign && NULL != request->secret_key);
        const char *http_api = request->using_multi ? (using_sign ? HTTPDNS_API_SIGN_RESOLVE : HTTPDNS_API_RESOLVE)
                                                    : (using_sign ? HTTPDNS_API_SIGN_D : HTTPDNS_API_D);
        sds url = sdsnew(http_scheme);
        url = sdscat(url, request->resolver);
        url = sdscat(url, "/");
        url = sdscat(url, request->account_id);
        url = sdscat(url, http_api);
        url = sdscat(url, "?host=");
        url = sdscat(url, request->host);
        url = sdscat(url, "&query=");
        url = sdscat(url, request->query_type);
        if (using_sign) {
            httpdns_signature_t *signature = httpdns_signature_create(request->host,
                                                                      request->secret_key,
                                                                      MAX_RESOLVE_SIGNATURE_OFFSET_TIME,
                                                                      httpdns_time_now());
            url = sdscat(url, "&s=");
            url = sdscat(url, signature->sign);
            url = sdscat(url, "&t=");
            url = sdscat(url, signature->timestamp);
            destroy_httpdns_signature(signature);
        }
        if (IS_NOT_BLANK_STRING(request->client_ip)) {
            url = sdscat(url, "&ip=");
            url = sdscat(url, request->client_ip);
        }
        url = sdscat(url, "&platform=linux&sdk_version=");
        url = sdscat(url, request->sdk_version);

        httpdns_http_context_t *http_context = httpdns_http_context_create(url, request->timeout_ms);
        httpdns_http_context_set_user_agent(http_context, request->user_agent);
        httpdns_http_context_set_private_data(http_context, resolve_param, NULL, NULL, NULL, NULL);
        httpdns_list_add(&http_contexts, http_context, NULL);

        sdsfree(url);

        http_context_size++;
    }

    httpdns_http_multiple_exchange(&http_contexts);

    for (int i = 0; i < http_context_size; i++) {
        httpdns_http_context_t *http_context = httpdns_list_get(&http_contexts, i);
        httpdns_resolve_param_t *resolve_param = http_context->private_data;
        if (NULL != resolve_param->http_finish_callback_func) {
            resolve_param->http_finish_callback_func(
                    http_context->response_body,
                    http_context->response_status,
                    http_context->response_rt_ms,
                    resolve_param->user_http_finish_callback_param);
        }
    }
    httpdns_list_free(&http_contexts, DATA_FREE_FUNC(httpdns_http_context_destroy));
    return HTTPDNS_SUCCESS;
}

httpdns_resolve_request_t *
httpdns_resolve_request_create(httpdns_config_t *config, char *host, char *resolver, char *query_type) {
    if (HTTPDNS_SUCCESS != httpdns_config_is_valid(config) || NULL == host) {
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(resolve_request, httpdns_resolve_request_t);
    resolve_request->host = sdsnew(host);
    resolve_request->cache_key = sdsnew(host);
    resolve_request->account_id = sdsnew(config->account_id);
    if (config->using_sign && IS_NOT_BLANK_STRING(config->secret_key)) {
        resolve_request->using_sign = config->using_sign;
        resolve_request->secret_key = sdsnew(config->secret_key);
    }
    resolve_request->using_https = config->using_https;
    resolve_request->using_cache = config->using_cache;
    if (NULL != config->user_agent) {
        resolve_request->user_agent = sdsnew(config->user_agent);
    }
    if (NULL != config->sdk_version) {
        resolve_request->sdk_version = sdsnew(config->sdk_version);
    }
    if (IS_NOT_BLANK_STRING(resolver)) {
        resolve_request->resolver = sdsnew(resolver);
    }
    if (IS_NOT_BLANK_STRING(query_type)) {
        resolve_request->query_type = sdsnew(query_type);
    }
    if (config->timeout_ms > 0) {
        resolve_request->timeout_ms = config->timeout_ms;
    }
    return resolve_request;
}


void httpdns_resolve_request_print(httpdns_resolve_request_t *request) {
    if (NULL == request) {
        printf("httpdns_resolve_request_t()");
    }
    printf("httpdns_resolve_request_t(host=%s", request->host);
    printf(",account_id=%s", request->account_id);
    printf(",secret_key=%s", request->secret_key);
    printf(",resolver=%s", request->resolver);
    printf(",query_type=%s", request->query_type);
    printf(",client_ip=%s", request->client_ip);
    printf(",sdk_version=%s", request->sdk_version);
    printf(",user_agent=%s", request->user_agent);
    printf(",sdns_params=%s", request->sdns_params);
    printf(",cache_key=%s", request->cache_key);
    printf(",using_https=%d", request->using_https);
    printf(",using_sign=%d", request->using_sign);
    printf(",using_multi=%d", request->using_multi);
    printf(",using_cache=%d", request->using_cache);
    printf(",timeout_ms=%d", request->timeout_ms);
    printf(")");
}

httpdns_resolve_request_t *httpdns_resolve_request_clone(httpdns_resolve_request_t *origin_resolve_request) {
    if (NULL == origin_resolve_request) {
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(new_resolve_request, httpdns_resolve_request_t);
    if (NULL != origin_resolve_request->host) {
        new_resolve_request->host = sdsnew(origin_resolve_request->host);
    }
    if (NULL != origin_resolve_request->account_id) {
        new_resolve_request->account_id = sdsnew(origin_resolve_request->account_id);
    }
    if (NULL != origin_resolve_request->secret_key) {
        new_resolve_request->secret_key = sdsnew(origin_resolve_request->secret_key);
    }
    if (NULL != origin_resolve_request->resolver) {
        new_resolve_request->resolver = sdsnew(origin_resolve_request->resolver);
    }
    if (NULL != origin_resolve_request->query_type) {
        new_resolve_request->query_type = sdsnew(origin_resolve_request->query_type);
    }
    if (NULL != origin_resolve_request->client_ip) {
        new_resolve_request->client_ip = sdsnew(origin_resolve_request->client_ip);
    }
    if (NULL != origin_resolve_request->sdk_version) {
        new_resolve_request->sdk_version = sdsnew(origin_resolve_request->sdk_version);
    }
    if (NULL != origin_resolve_request->user_agent) {
        new_resolve_request->user_agent = sdsnew(origin_resolve_request->user_agent);
    }
    if (NULL != origin_resolve_request->sdns_params) {
        new_resolve_request->sdns_params = sdsnew(origin_resolve_request->sdns_params);
    }
    if (NULL != origin_resolve_request->cache_key) {
        new_resolve_request->cache_key = sdsnew(origin_resolve_request->cache_key);
    }
    new_resolve_request->using_https = origin_resolve_request->using_https;
    new_resolve_request->using_sign = origin_resolve_request->using_sign;
    new_resolve_request->using_multi = origin_resolve_request->using_multi;
    new_resolve_request->using_cache = origin_resolve_request->using_cache;
    new_resolve_request->timeout_ms = origin_resolve_request->timeout_ms;
    return new_resolve_request;
}

void httpdns_resolve_request_append_sdns_params(httpdns_resolve_request_t *request, char *key, char *value) {
    if (NULL == request || IS_BLANK_STRING(key) || IS_BLANK_STRING(value)) {
        return;
    }
    if (NULL == request->sdns_params) {
        request->sdns_params = sdsempty();
    }
    request->sdns_params = sdscat(request->sdns_params, key);
    request->sdns_params = sdscat(request->sdns_params, "=");
    request->sdns_params = sdscat(request->sdns_params, value);
}

void httpdns_resolve_request_set_host(httpdns_resolve_request_t *request, char *host) {
    HTTPDNS_SET_STRING_FIELD(request, host, host);
}

void httpdns_resolve_request_set_account_id(httpdns_resolve_request_t *request, char *account_id) {
    HTTPDNS_SET_STRING_FIELD(request, account_id, account_id);
}

void httpdns_resolve_request_set_secret_key(httpdns_resolve_request_t *request, char *secret_key) {
    HTTPDNS_SET_STRING_FIELD(request, secret_key, secret_key);
}

void httpdns_resolve_request_set_resolver(httpdns_resolve_request_t *request, char *resolver) {
    HTTPDNS_SET_STRING_FIELD(request, resolver, resolver);
}

void httpdns_resolve_request_set_client_ip(httpdns_resolve_request_t *request, char *client_ip) {
    HTTPDNS_SET_STRING_FIELD(request, client_ip, client_ip);
}

void httpdns_resolve_request_set_sdk_version(httpdns_resolve_request_t *request, char *sdk_version) {
    HTTPDNS_SET_STRING_FIELD(request, sdk_version, sdk_version);
}

void httpdns_resolve_request_set_user_agent(httpdns_resolve_request_t *request, char *user_agent) {
    HTTPDNS_SET_STRING_FIELD(request, user_agent, user_agent);
}

void httpdns_resolve_request_set_cache_key(httpdns_resolve_request_t *request, char *cache_key) {
    HTTPDNS_SET_STRING_FIELD(request, cache_key, cache_key);
}

void httpdns_resolve_request_set_query_type(httpdns_resolve_request_t *request, char *query_type) {
    HTTPDNS_SET_STRING_FIELD(request, query_type, query_type);
}

void httpdns_resolve_request_set_timeout_ms(httpdns_resolve_request_t *request, int32_t timeout_ms) {
    if (NULL != request && timeout_ms > 0) {
        request->timeout_ms = timeout_ms;
    }
}

void httpdns_resolve_request_set_using_https(httpdns_resolve_request_t *request, bool using_https) {
    if (NULL != request) {
        request->using_https = using_https;
    }
}

void httpdns_resolve_request_set_using_sign(httpdns_resolve_request_t *request, bool using_sign) {
    if (NULL != request) {
        request->using_sign = using_sign;
    }
}

void httpdns_resolve_request_set_using_multi(httpdns_resolve_request_t *request, bool using_multi) {
    if (NULL != request) {
        request->using_multi = using_multi;
        if (NULL != request->cache_key) {
            sdsfree(request->cache_key);
            request->cache_key = NULL;
        }
    }
}


httpdns_resolve_context_t *httpdns_resolve_context_create(httpdns_resolve_request_t *request) {
    if (NULL == request) {
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(resolve_context, httpdns_resolve_context_t);
    resolve_context->request = httpdns_resolve_request_clone(request);
    httpdns_list_init(&resolve_context->result);
    return resolve_context;
}

httpdns_resolve_context_t *httpdns_resolve_context_clone(httpdns_resolve_context_t *origin_context) {
    if (NULL == origin_context) {
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(resolve_context, httpdns_resolve_context_t);
    resolve_context->request = httpdns_resolve_request_clone(origin_context->request);
    httpdns_list_init(&resolve_context->result);
    httpdns_list_dup(&resolve_context->result, &origin_context->result, DATA_CLONE_FUNC(httpdns_resolve_result_clone));
    return resolve_context;
}

void httpdns_resolve_context_destroy(httpdns_resolve_context_t *resolve_context) {
    if (NULL == resolve_context) {
        return;
    }
    if (NULL != resolve_context->request) {
        httpdns_resolve_request_destroy(resolve_context->request);
    }
    httpdns_list_free(&resolve_context->result, DATA_FREE_FUNC(httpdns_resolve_result_destroy));
    free(resolve_context);
}

void httpdns_resolve_request_destroy(httpdns_resolve_request_t *request) {
    if (NULL == request) {
        return;
    }
    if (NULL != request->host) {
        sdsfree(request->host);
    }
    if (NULL != request->account_id) {
        sdsfree(request->account_id);
    }
    if (NULL != request->secret_key) {
        sdsfree(request->secret_key);
    }
    if (NULL != request->resolver) {
        sdsfree(request->resolver);
    }
    if (NULL != request->query_type) {
        sdsfree(request->query_type);
    }
    if (NULL != request->client_ip) {
        sdsfree(request->client_ip);
    }
    if (NULL != request->sdk_version) {
        sdsfree(request->sdk_version);
    }
    if (NULL != request->user_agent) {
        sdsfree(request->user_agent);
    }
    if (NULL != request->sdns_params) {
        sdsfree(request->sdns_params);
    }
    if (NULL != request->cache_key) {
        sdsfree(request->cache_key);
    }
    free(request);
}

void httpdns_resolve_result_destroy(httpdns_resolve_result_t *result) {
    if (NULL == result) {
        return;
    }
    if (NULL != result->host) {
        sdsfree(result->host);
    }
    if (NULL != result->client_ip) {
        sdsfree(result->client_ip);
    }
    if (NULL != result->extra) {
        sdsfree(result->extra);
    }
    if (NULL != result->cache_key) {
        sdsfree(result->cache_key);
    }
    httpdns_list_free(&result->ips, DATA_FREE_FUNC(httpdns_ip_destroy));
    httpdns_list_free(&result->ipsv6, DATA_FREE_FUNC(httpdns_ip_destroy));
    free(result);
}


void httpdns_resolve_result_set_cache_key(httpdns_resolve_result_t *result, char *cache_key) {
    HTTPDNS_SET_STRING_FIELD(result, cache_key, cache_key);
}

void httpdns_resolve_result_set_hit_cache(httpdns_resolve_result_t *result, bool hit_cache) {
    if (NULL != result) {
        result->hit_cache = hit_cache;
    }
}
//
//httpdns_resolve_request_t *clone_httpdns_resolve_request(httpdns_resolve_request_t *origin_request) {
//    if (NULL == origin_request) {
//        return NULL;
//    }
//    httpdns_resolve_request_t *request = (httpdns_resolve_request_t *) malloc(
//            sizeof(httpdns_resolve_request_t));
//    memset(request, 0, sizeof(httpdns_resolve_request_t));
//    if (IS_NOT_BLANK_STRING(origin_request->host)) {
//        request->host = sdsnew(origin_request->host);
//    }
//    if (IS_NOT_BLANK_STRING(origin_request->sdns_params)) {
//        request->sdns_params = sdsnew(origin_request->sdns_params);
//    }
//    if (IS_NOT_BLANK_STRING(origin_request->cache_key)) {
//        request->cache_key = sdsnew(origin_request->cache_key);
//    }
//    request->query_type = origin_request->query_type;
//    request->timeout_ms = origin_request->timeout_ms;
//    request->hit_cache = origin_request->hit_cache;
//    return request;
//}
//
//
//static void resolve_requests_to_http_requests(struct list_head *http_requests, httpdns_resolve_task_t *task) {
//    struct list_head *resolve_requests = &task->requests;
//    httpdns_client_t *resolver = task->client;
//    httpdns_config_t *config = resolver->config;
//    httpdns_cache_table_t *cache_table = resolver->cache;
//    size_t resolve_req_nums = httpdns_list_size(resolve_requests);
//    for (int i = 0; i < resolve_req_nums; i++) {
//        httpdns_resolve_request_t *resolve_request = httpdns_list_get(resolve_requests, i);
//        if (config->using_cache) {
//            httpdns_cache_entry_t *entry = httpdns_cache_get_entry(cache_table, resolve_request->cache_key);
//            if (NULL != entry) {
//                httpdns_resolve_result_t *result = httpdns_resolve_result_clone(entry);
//                result->hit_cache = true;
//                httpdns_list_add(&task->results, entry, NULL);
//                continue;
//            }
//        }
//        httpdns_http_request_t *http_request = resolve_request_to_http_request(resolve_request, resolver);
//        httpdns_list_add(http_requests, http_request, NULL);
//    }
//}
//
//static httpdns_resolve_result_t *
//raw_single_result_to_resolve_result(httpdns_single_resolve_response_t *raw_single_resolve_result) {
//    if (NULL == raw_single_resolve_result) {
//        return NULL;
//    }
//    HTTPDNS_NEW_OBJECT_IN_HEAP(resolve_result, httpdns_resolve_result_t);
//    resolve_result->hit_cache = false;
//    resolve_result->query_ts = httpdns_time_now();
//    resolve_result->origin_ttl = raw_single_resolve_result->origin_ttl;
//    resolve_result->ttl = raw_single_resolve_result->ttl;
//    if (NULL != raw_single_resolve_result->extra) {
//        resolve_result->extra = sdsnew(raw_single_resolve_result->extra);
//    }
//    if (NULL != raw_single_resolve_result->client_ip) {
//        resolve_result->client_ip = sdsnew(raw_single_resolve_result->client_ip);
//    }
//    if (NULL != raw_single_resolve_result->host) {
//        resolve_result->host = sdsnew(raw_single_resolve_result->host);
//    }
//    httpdns_list_dup(&resolve_result->ips, &raw_single_resolve_result->ips, DATA_CLONE_FUNC(httpdns_ip_clone));
//    httpdns_list_dup(&resolve_result->ipsv6, &raw_single_resolve_result->ipsv6, DATA_CLONE_FUNC(httpdns_ip_clone));
//    return resolve_result;
//}
//
//static httpdns_resolve_result_t *http_response_to_resolve_result(httpdns_http_context_t *http_response) {
//    if (NULL == http_response) {
//        return NULL;
//    }
//    if (http_response->response_status == HTTP_STATUS_OK) {
//        httpdns_single_resolve_response_t *raw_single_resolve_result = httpdns_response_parse_single_resolve(
//                http_response->response_body);
//        if (NULL == raw_single_resolve_result) {
//            return NULL;
//        }
//        httpdns_resolve_result_t *resolve_result = raw_single_result_to_resolve_result(
//                raw_single_resolve_result);
//        if (NULL != http_response->cache_key) {
//            resolve_result->cache_key = sdsnew(http_response->cache_key);
//        }
//        httpdns_single_resolve_response_destroy(raw_single_resolve_result);
//        return resolve_result;
//    }
//    return NULL;
//}
//
//
//void httpdns_resolve_result_create(httpdns_resolve_result_t *result) {
//    if (NULL == result) {
//        return;
//    }
//    if (IS_NOT_BLANK_STRING(result->host)) {
//        sdsfree(result->host);
//    }
//    if (IS_NOT_BLANK_STRING(result->client_ip)) {
//        sdsfree(result->client_ip);
//    }
//    if (IS_NOT_BLANK_STRING(result->extra)) {
//        sdsfree(result->extra);
//    }
//    if (IS_NOT_BLANK_STRING(result->cache_key)) {
//        sdsfree(result->cache_key);
//    }
//    httpdns_list_free(&result->ips, DATA_FREE_FUNC(httpdns_ip_destroy));
//    httpdns_list_free(&result->ipsv6, DATA_FREE_FUNC(httpdns_ip_destroy));
//    free(result);
//}
//
//

void httpdns_resolve_result_print(httpdns_resolve_result_t *result) {
    if (NULL == result) {
        return;
    }
    char query_ts[32];
    httpdns_time_to_string(result->query_ts, query_ts, 32);
    printf("httpdns_resolve_result_t(");
    printf("host=%s, client_ip=%s, extra=%s, origin_ttl=%d, ttl=%d, private_data=%s, hit_cache=%d, query_ts=%s",
           result->host, result->client_ip, result->extra, result->origin_ttl, result->ttl, result->cache_key,
           result->hit_cache, query_ts);
    printf(",ips=");
    httpdns_list_print(&result->ips, DATA_PRINT_FUNC(httpdns_ip_print));
    printf(",ipsv6=");
    httpdns_list_print(&result->ipsv6, DATA_PRINT_FUNC(httpdns_ip_print));
    printf(")");
}


httpdns_resolve_result_t *httpdns_resolve_result_clone(httpdns_resolve_result_t *origin_result) {
    if (NULL == origin_result) {
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(new_result, httpdns_resolve_result_t);
    memset(new_result, 0, sizeof(httpdns_resolve_result_t));
    if (NULL != origin_result->host) {
        new_result->host = sdsnew(origin_result->host);
    }
    if (NULL != origin_result->client_ip) {
        new_result->client_ip = sdsnew(origin_result->client_ip);
    }
    if (NULL != origin_result->extra) {
        new_result->extra = sdsnew(origin_result->extra);
    }
    if (NULL != origin_result->cache_key) {
        new_result->cache_key = sdsnew(origin_result->cache_key);
    }
    new_result->ttl = origin_result->ttl;
    new_result->origin_ttl = origin_result->origin_ttl;
    new_result->query_ts = origin_result->query_ts;
    new_result->hit_cache = origin_result->hit_cache;
    httpdns_list_dup(&new_result->ips, &origin_result->ips, DATA_CLONE_FUNC(httpdns_ip_clone));
    httpdns_list_dup(&new_result->ipsv6, &origin_result->ipsv6, DATA_CLONE_FUNC(httpdns_ip_clone));
    return new_result;
}

httpdns_resolve_param_t *httpdns_resolve_param_create(httpdns_resolve_request_t *request) {
    if (NULL == request) {
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(resolve_param, httpdns_resolve_param_t);
    resolve_param->request = httpdns_resolve_request_clone(request);
    return resolve_param;
}

void httpdns_resolve_param_destroy(httpdns_resolve_param_t *resolve_param) {
    if (NULL == resolve_param) {
        return;
    }
    if (NULL != resolve_param->request) {
        httpdns_resolve_request_destroy(resolve_param->request);
    }
    if (NULL != resolve_param->callback_param_free_func) {
        resolve_param->callback_param_free_func(resolve_param->user_http_finish_callback_param);
    }
    free(resolve_param);
}

httpdns_resolve_param_t *httpdns_resolve_param_clone(httpdns_resolve_param_t *origin_resolve_param) {
    if (NULL == origin_resolve_param) {
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(new_resolve_param, httpdns_resolve_param_t);
    if (NULL != origin_resolve_param->request) {
        new_resolve_param->request = httpdns_resolve_request_clone(origin_resolve_param->request);
    }
    new_resolve_param->user_http_finish_callback_param = origin_resolve_param->user_http_finish_callback_param;
    new_resolve_param->http_finish_callback_func = origin_resolve_param->http_finish_callback_func;
    return new_resolve_param;
}