//
// Created by cagaoshuai on 2024/1/18.
//

#include "httpdns_resolver.h"
#include "httpdns_cache.h"


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
    return resolver;
}


httpdns_resolve_request_t *create_httpdns_resolve_request(char *host, dns_type_t dns_type) {
    if (IS_BLANK_SDS(host)) {
        return NULL;
    }
    httpdns_resolve_request_t *request = (httpdns_resolve_request_t *) malloc(sizeof(httpdns_resolve_request_t));
    memset(request, 0, sizeof(httpdns_resolve_request_t));
    request->host = sdsnew(host);
    request->dns_type = dns_type;
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
}


httpdns_resolve_result_t *resolve(httpdns_resolve_task_t *task) {
    return NULL;
}


void destroy_httpdns_resolver(httpdns_resolver_t *resolver) {
    if (NULL == resolver) {
        return;
    }
    if (NULL != resolver->config) {
        destroy_httpdns_config(resolver->config);
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
}
