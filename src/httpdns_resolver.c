//
// Created by cagaoshuai on 2024/1/18.
//

#include "httpdns_resolver.h"
#include "httpdns_cache.h"
#include "httpdns_sign.h"
#include "response_parser.h"
#include "httpdns_memory.h"
#include "httpdns_time.h"


httpdns_resolver_t *create_httpdns_resolver(httpdns_config_t *config) {
    if (httpdns_config_is_valid(config) != HTTPDNS_SUCCESS) {
        return NULL;
    }
    httpdns_resolver_t *resolver = (httpdns_resolver_t *) malloc(sizeof(httpdns_resolver_t));
    memset(resolver, 0, sizeof(httpdns_resolver_t));
    resolver->config = config;
    resolver->net_stack_detector = create_net_stack_detector();
    net_stack_detector_set_probe_domain(resolver->net_stack_detector, config->probe_domain);
    resolver->cache = create_httpdns_cache_table();
    resolver->scheduler = create_httpdns_scheduler(config);
    httpdns_scheduler_refresh_resolve_servers(resolver->scheduler);
    return resolver;
}


httpdns_resolve_request_t *create_httpdns_resolve_request(char *host, resolve_type_t dns_type, char *cache_key) {
    if (IS_BLANK_SDS(host)) {
        return NULL;
    }
    httpdns_resolve_request_t *request = (httpdns_resolve_request_t *) malloc(sizeof(httpdns_resolve_request_t));
    memset(request, 0, sizeof(httpdns_resolve_request_t));
    request->host = sdsnew(host);
    request->dns_type = dns_type;
    if (IS_BLANK_SDS(cache_key)) {
        request->cache_key = sdsnew(host);
    } else {
        request->cache_key = sdsnew(cache_key);
    }
    request->hit_cache = false;
    return request;
}

void httpdns_resolve_request_append_sdns_params(httpdns_resolve_request_t *request, char *key, char *value) {
    if (NULL == request || IS_BLANK_SDS(key) || IS_BLANK_SDS(value)) {
        return;
    }
    if (NULL == request->sdns_params) {
        request->sdns_params = sdsempty();
    }
    request->sdns_params = sdscat(request->sdns_params, key);
    request->sdns_params = sdscat(request->sdns_params, "=");
    request->sdns_params = sdscat(request->sdns_params, value);
}

void httpdns_resolve_request_set_cache_key(httpdns_resolve_request_t *request, char *cache_key) {
    if (NULL == request || IS_BLANK_SDS(cache_key)) {
        return;
    }
    request->cache_key = sdsnew(cache_key);
}

void destroy_httpdns_resolve_request(httpdns_resolve_request_t *request) {
    if (NULL == request) {
        return;
    }
    if (IS_NOT_BLANK_SDS(request->host)) {
        sdsfree(request->host);
    }
    if (IS_NOT_BLANK_SDS(request->sdns_params)) {
        sdsfree(request->sdns_params);
    }
    if (IS_NOT_BLANK_SDS(request->cache_key)) {
        sdsfree(request->cache_key);
    }
    free(request);
}

httpdns_resolve_request_t *clone_httpdns_resolve_request(httpdns_resolve_request_t *origin_request) {
    if (NULL == origin_request) {
        return NULL;
    }
    httpdns_resolve_request_t *request = (httpdns_resolve_request_t *) malloc(sizeof(httpdns_resolve_request_t));
    memset(request, 0, sizeof(httpdns_resolve_request_t));
    if (IS_NOT_BLANK_SDS(origin_request->host)) {
        request->host = sdsnew(origin_request->host);
    }
    if (IS_NOT_BLANK_SDS(origin_request->sdns_params)) {
        request->sdns_params = sdsnew(origin_request->sdns_params);
    }
    if (IS_NOT_BLANK_SDS(origin_request->cache_key)) {
        request->cache_key = sdsnew(origin_request->cache_key);
    }
    request->dns_type = origin_request->dns_type;
    request->timeout_ms = origin_request->timeout_ms;
    request->hit_cache = origin_request->hit_cache;
    return request;
}


httpdns_resolve_task_t *create_httpdns_resolve_task(httpdns_resolver_t *resolver) {
    if (NULL == resolver) {
        return NULL;
    }
    httpdns_resolve_task_t *task = (httpdns_resolve_task_t *) malloc(sizeof(httpdns_resolve_task_t));
    task->resolver = resolver;
    httpdns_list_init(&task->requests);
    httpdns_list_init(&task->results);
    return task;
}

void httpdns_resolve_task_add_request(httpdns_resolve_task_t *task, httpdns_resolve_request_t *request) {
    if (NULL == task || NULL == request) {
        return;
    }
    httpdns_list_add(&task->requests, request, NULL);
}

void httpdns_resolve_task_add_result(httpdns_resolve_task_t *task, httpdns_resolve_result_t *result) {
    if (NULL == task || NULL == result) {
        return;
    }
    httpdns_list_add(&task->results, result, NULL);
}

void destroy_httpdns_resolve_task(httpdns_resolve_task_t *task) {
    if (NULL == task) {
        return;
    }
    httpdns_list_free(&task->requests, DATA_FREE_FUNC(destroy_httpdns_resolve_request));
    httpdns_list_free(&task->results, DATA_FREE_FUNC(destroy_httpdns_resolve_result));
    free(task);
}

static httpdns_http_request_t *
resolve_request_to_http_request(httpdns_resolve_request_t *resolve_request, httpdns_resolver_t *resolver) {
    if (NULL == resolve_request || NULL == resolver) {
        return NULL;
    }
    httpdns_config_t *config = resolver->config;
    httpdns_scheduler_t *scheduler = resolver->scheduler;
    if (NULL == config || NULL == scheduler) {
        return NULL;
    }
    const char *http_scheme = config->using_https ? HTTPS_SCHEME : HTTP_SCHEME;
    const char *http_api = config->using_sign ? HTTPDNS_API_SIGN_D : HTTPDNS_API_D;
    char *resolve_server_ip;
    httpdns_scheduler_get_resolve_server(scheduler, &resolve_server_ip);
    if (NULL == resolve_server_ip) {
        return NULL;
    }
    httpdns_http_request_t *http_request = (httpdns_http_request_t *) malloc(sizeof(httpdns_http_request_t));
    http_request->timeout_ms = config->timeout_ms;
    http_request->cache_key = sdsnew(resolve_request->cache_key);
    sds url = sdsnew(http_scheme);
    url = sdscat(url, resolve_server_ip);
    url = sdscat(url, "/");
    url = sdscat(url, config->account_id);
    url = sdscat(url, http_api);
    url = sdscat(url, "?host=");
    url = sdscat(url, resolve_request->host);
    if (config->using_sign && NULL != config->secret_key) {
        httpdns_signature_t *signature = create_httpdns_signature(resolve_request->host, config->secret_key);
        url = sdscat(url, "&s=");
        url = sdscat(url, signature->sign);
        url = sdscat(url, "&t=");
        url = sdscat(url, signature->timestamp);
        destroy_httpdns_signature(signature);
    }
    url = sdscat(url, "&platform=linux&sdk_version=");
    url = sdscat(url, config->sdk_version);
    http_request->url = url;
    sdsfree(resolve_server_ip);
    return http_request;
}

static void resolve_requests_to_http_requests(struct list_head *http_requests, httpdns_resolve_task_t *task) {
    struct list_head *resolve_requests = &task->requests;
    httpdns_resolver_t *resolver = task->resolver;
    httpdns_config_t *config = resolver->config;
    httpdns_cache_table_t *cache_table = resolver->cache;
    size_t resolve_req_nums = httpdns_list_size(resolve_requests);
    for (int i = 0; i < resolve_req_nums; i++) {
        httpdns_resolve_request_t *resolve_request = httpdns_list_get(resolve_requests, i);
        if (config->using_cache) {
            httpdns_cache_entry_t *entry = httpdns_cache_get_entry(cache_table, resolve_request->cache_key);
            if (NULL != entry) {
                httpdns_resolve_result_t *result = clone_httpdns_resolve_result(entry);
                result->hit_cache = true;
                httpdns_list_add(&task->results, entry, NULL);
                continue;
            }
        }
        httpdns_http_request_t *http_request = resolve_request_to_http_request(resolve_request, resolver);
        httpdns_list_add(http_requests, http_request, NULL);
    }
}

static httpdns_resolve_result_t *
raw_single_result_to_resolve_result(httpdns_raw_single_resolve_result_t *raw_single_resolve_result) {
    if (NULL == raw_single_resolve_result) {
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(resolve_result, httpdns_resolve_result_t);
    resolve_result->hit_cache = false;
    resolve_result->query_ts = httpdns_time_now();
    resolve_result->origin_ttl = raw_single_resolve_result->origin_ttl;
    resolve_result->ttl = raw_single_resolve_result->ttl;
    if (NULL != raw_single_resolve_result->extra) {
        resolve_result->extra = sdsnew(raw_single_resolve_result->extra);
    }
    if (NULL != raw_single_resolve_result->client_ip) {
        resolve_result->client_ip = sdsnew(raw_single_resolve_result->client_ip);
    }
    if (NULL != raw_single_resolve_result->host) {
        resolve_result->host = sdsnew(raw_single_resolve_result->host);
    }
    httpdns_list_dup(&resolve_result->ips, &raw_single_resolve_result->ips, DATA_CLONE_FUNC(clone_httpdns_ip));
    httpdns_list_dup(&resolve_result->ipsv6, &raw_single_resolve_result->ipsv6, DATA_CLONE_FUNC(clone_httpdns_ip));
    return resolve_result;
}

static httpdns_resolve_result_t *http_response_to_resolve_result(httpdns_http_response_t *http_response) {
    if (NULL == http_response) {
        return NULL;
    }
    if (http_response->http_status == HTTP_STATUS_OK) {
        httpdns_raw_single_resolve_result_t *raw_single_resolve_result = parse_single_resolve_result(
                http_response->body);
        if (NULL == raw_single_resolve_result) {
            return NULL;
        }
        httpdns_resolve_result_t *resolve_result = raw_single_result_to_resolve_result(raw_single_resolve_result);
        if (NULL != http_response->cache_key) {
            resolve_result->cache_key = sdsnew(http_response->cache_key);
        }
        destroy_httpdns_raw_single_resolve_result(raw_single_resolve_result);
        return resolve_result;
    }
    return NULL;
}

int32_t resolve(httpdns_resolve_task_t *task) {
    if (NULL == task || NULL == task->resolver || IS_EMPTY_LIST(&task->requests)) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    struct list_head http_requests;
    httpdns_list_init(&http_requests);
    struct list_head http_responses;
    httpdns_list_init(&http_responses);

    resolve_requests_to_http_requests(&http_requests, task);
    if (IS_EMPTY_LIST(&http_requests)) {
        return HTTPDNS_SUCCESS;
    }
    int32_t ret = httpdns_http_multiple_request_exchange(&http_requests, &http_responses);
    httpdns_list_free(&http_requests, DATA_FREE_FUNC(destroy_httpdns_http_request));
    if (ret != HTTPDNS_SUCCESS || IS_EMPTY_LIST(&http_responses)) {
        return HTTPDNS_CORRECT_RESPONSE_EMPTY;
    }
    httpdns_list_dup(&task->results, &http_responses, DATA_CLONE_FUNC(http_response_to_resolve_result));
    httpdns_list_free(&http_responses, DATA_FREE_FUNC(destroy_httpdns_http_response));
    return HTTPDNS_SUCCESS;
}


void destroy_httpdns_resolver(httpdns_resolver_t *resolver) {
    if (NULL == resolver) {
        return;
    }
    if (NULL != resolver->cache) {
        destroy_httpdns_cache_table(resolver->cache);
    }
    if (NULL != resolver->scheduler) {
        destroy_httpdns_scheduler(resolver->scheduler);
    }
    if (NULL != resolver->net_stack_detector) {
        destroy_net_stack_detector(resolver->net_stack_detector);
    }
    free(resolver);
}
