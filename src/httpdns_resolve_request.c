//
// Created by caogaoshuai on 2024/1/29.
//
#include "httpdns_resolve_request.h"
#include "httpdns_error_type.h"
#include "httpdns_log.h"
#include "httpdns_sds.h"
#include "httpdns_memory.h"


int32_t httpdns_resolve_request_valid(const httpdns_resolve_request_t *request) {
    if (NULL == request) {
        httpdns_log_info("resolve request valid failed, request is NULL");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (httpdns_string_is_blank(request->host)) {
        httpdns_log_info("resolve request valid failed, host is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (httpdns_string_is_blank(request->account_id)) {
        httpdns_log_info("resolve request valid failed, account_id is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (httpdns_string_is_blank(request->resolver)) {
        httpdns_log_info("resolve request valid failed, resolver is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (httpdns_string_is_blank(request->query_type)) {
        httpdns_log_info("resolve request valid failed, query_type is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (httpdns_string_is_blank(request->user_agent)) {
        httpdns_log_info("resolve request valid failed, user_agent is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (httpdns_string_is_blank(request->sdk_version)) {
        httpdns_log_info("resolve request valid failed, sdk_version is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (request->using_multi && httpdns_string_is_not_blank(request->cache_key)) {
        httpdns_log_info(
                "resolve request valid failed, when using httpdns api resolve or sign_resolve, cache_key must be blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (request->timeout_ms <= 0) {
        httpdns_log_info("resolve request valid failed, timeout_ms is less than 0");
        return HTTPDNS_PARAMETER_ERROR;
    }
    return HTTPDNS_SUCCESS;
}


httpdns_resolve_request_t *
httpdns_resolve_request_new(httpdns_config_t *config, const char *host, const char *resolver, const char *query_type) {
    if (HTTPDNS_SUCCESS != httpdns_config_valid(config) || NULL == host) {
        return NULL;
    }
    httpdns_new_object_in_heap(resolve_request, httpdns_resolve_request_t);
    resolve_request->host = httpdns_sds_new(host);
    resolve_request->cache_key = httpdns_sds_new(host);
    resolve_request->account_id = httpdns_sds_new(config->account_id);
    if (config->using_sign && httpdns_string_is_not_blank(config->secret_key)) {
        resolve_request->using_sign = config->using_sign;
        resolve_request->secret_key = httpdns_sds_new(config->secret_key);
    }
    resolve_request->using_https = config->using_https;
    resolve_request->using_cache = config->using_cache;
    if (NULL != config->user_agent) {
        resolve_request->user_agent = httpdns_sds_new(config->user_agent);
    }
    if (NULL != config->sdk_version) {
        resolve_request->sdk_version = httpdns_sds_new(config->sdk_version);
    }
    if (httpdns_string_is_not_blank(resolver)) {
        resolve_request->resolver = httpdns_sds_new(resolver);
    }
    if (httpdns_string_is_not_blank(query_type)) {
        resolve_request->query_type = httpdns_sds_new(query_type);
    }
    if (config->timeout_ms > 0) {
        resolve_request->timeout_ms = config->timeout_ms;
    }
    return resolve_request;
}


httpdns_sds_t httpdns_resolve_request_to_string(const httpdns_resolve_request_t *request) {
    if (NULL == request) {
        return httpdns_sds_new("httpdns_resolve_request_t()");
    }
    httpdns_sds_t dst_str = httpdns_sds_new("httpdns_resolve_request_t(host=");
    httpdns_sds_cat_easily(dst_str, request->host);
    httpdns_sds_cat_easily(dst_str, ",account_id=");
    httpdns_sds_cat_easily(dst_str, request->account_id);
    httpdns_sds_cat_easily(dst_str, ",secret_key=");
    httpdns_sds_cat_easily(dst_str, request->secret_key);
    httpdns_sds_cat_easily(dst_str, ",resolver=");
    httpdns_sds_cat_easily(dst_str, request->resolver);
    httpdns_sds_cat_easily(dst_str, ",query_type=");
    httpdns_sds_cat_easily(dst_str, request->query_type);
    httpdns_sds_cat_easily(dst_str, ",client_ip=");
    httpdns_sds_cat_easily(dst_str, request->client_ip);

    httpdns_sds_cat_easily(dst_str, ",sdk_version=");
    httpdns_sds_cat_easily(dst_str, request->sdk_version);
    httpdns_sds_cat_easily(dst_str, ",user_agent=");
    httpdns_sds_cat_easily(dst_str, request->user_agent);
    httpdns_sds_cat_easily(dst_str, ",sdns_params=");
    httpdns_sds_cat_easily(dst_str, request->sdns_params);
    httpdns_sds_cat_easily(dst_str, ",cache_key=");
    httpdns_sds_cat_easily(dst_str, request->cache_key);

    httpdns_sds_cat_easily(dst_str, ",using_https=");
    httpdns_sds_cat_int(dst_str, request->using_https)
    httpdns_sds_cat_easily(dst_str, ",using_sign=");
    httpdns_sds_cat_int(dst_str, request->using_sign)
    httpdns_sds_cat_easily(dst_str, ",using_multi=");
    httpdns_sds_cat_int(dst_str, request->using_multi)
    httpdns_sds_cat_easily(dst_str, ",using_cache=");
    httpdns_sds_cat_int(dst_str, request->using_cache)
    httpdns_sds_cat_easily(dst_str, ",timeout_ms=");
    httpdns_sds_cat_int(dst_str, request->timeout_ms)
    httpdns_sds_cat_easily(dst_str, ")");
    return dst_str;
}

httpdns_resolve_request_t *httpdns_resolve_request_clone(const httpdns_resolve_request_t *origin_resolve_request) {
    if (NULL == origin_resolve_request) {
        httpdns_log_info("resolve request clone failed, origin resolve reqeust is NULL");
        return NULL;
    }
    httpdns_new_object_in_heap(new_resolve_request, httpdns_resolve_request_t);
    if (NULL != origin_resolve_request->host) {
        new_resolve_request->host = httpdns_sds_new(origin_resolve_request->host);
    }
    if (NULL != origin_resolve_request->account_id) {
        new_resolve_request->account_id = httpdns_sds_new(origin_resolve_request->account_id);
    }
    if (NULL != origin_resolve_request->secret_key) {
        new_resolve_request->secret_key = httpdns_sds_new(origin_resolve_request->secret_key);
    }
    if (NULL != origin_resolve_request->resolver) {
        new_resolve_request->resolver = httpdns_sds_new(origin_resolve_request->resolver);
    }
    if (NULL != origin_resolve_request->query_type) {
        new_resolve_request->query_type = httpdns_sds_new(origin_resolve_request->query_type);
    }
    if (NULL != origin_resolve_request->client_ip) {
        new_resolve_request->client_ip = httpdns_sds_new(origin_resolve_request->client_ip);
    }
    if (NULL != origin_resolve_request->sdk_version) {
        new_resolve_request->sdk_version = httpdns_sds_new(origin_resolve_request->sdk_version);
    }
    if (NULL != origin_resolve_request->user_agent) {
        new_resolve_request->user_agent = httpdns_sds_new(origin_resolve_request->user_agent);
    }
    if (NULL != origin_resolve_request->sdns_params) {
        new_resolve_request->sdns_params = httpdns_sds_new(origin_resolve_request->sdns_params);
    }
    if (NULL != origin_resolve_request->cache_key) {
        new_resolve_request->cache_key = httpdns_sds_new(origin_resolve_request->cache_key);
    }
    new_resolve_request->using_https = origin_resolve_request->using_https;
    new_resolve_request->using_sign = origin_resolve_request->using_sign;
    new_resolve_request->using_multi = origin_resolve_request->using_multi;
    new_resolve_request->using_cache = origin_resolve_request->using_cache;
    new_resolve_request->timeout_ms = origin_resolve_request->timeout_ms;
    new_resolve_request->user_callback_param = origin_resolve_request->user_callback_param;
    new_resolve_request->complete_callback_func = origin_resolve_request->complete_callback_func;
    return new_resolve_request;
}

void
httpdns_resolve_request_append_sdns_params(httpdns_resolve_request_t *request, const char *key, const char *value) {
    if (NULL == request || httpdns_string_is_blank(key) || httpdns_string_is_blank(value)) {
        httpdns_log_info("append sdns param failed, request or key or value is NULL");
        return;
    }
    if (NULL == request->sdns_params) {
        request->sdns_params = httpdns_sds_empty();
    }
    httpdns_sds_cat_easily(request->sdns_params, "&sdns-");
    httpdns_sds_cat_easily(request->sdns_params, key);
    httpdns_sds_cat_easily(request->sdns_params, "=");
    httpdns_sds_cat_easily(request->sdns_params, value);
}

void httpdns_resolve_request_set_host(httpdns_resolve_request_t *request, const char *host) {
    httpdns_set_string_field(request, host, host);
}

void httpdns_resolve_request_set_account_id(httpdns_resolve_request_t *request, const char *account_id) {
    httpdns_set_string_field(request, account_id, account_id);
}

void httpdns_resolve_request_set_secret_key(httpdns_resolve_request_t *request, const char *secret_key) {
    httpdns_set_string_field(request, secret_key, secret_key);
}

void httpdns_resolve_request_set_resolver(httpdns_resolve_request_t *request, const char *resolver) {
    httpdns_set_string_field(request, resolver, resolver);
}

void httpdns_resolve_request_set_client_ip(httpdns_resolve_request_t *request, const char *client_ip) {
    httpdns_set_string_field(request, client_ip, client_ip);
}

void httpdns_resolve_request_set_sdk_version(httpdns_resolve_request_t *request, const char *sdk_version) {
    httpdns_set_string_field(request, sdk_version, sdk_version);
}

void httpdns_resolve_request_set_user_agent(httpdns_resolve_request_t *request, const char *user_agent) {
    httpdns_set_string_field(request, user_agent, user_agent);
}

void httpdns_resolve_request_set_cache_key(httpdns_resolve_request_t *request, const char *cache_key) {
    httpdns_set_string_field(request, cache_key, cache_key);
}

void httpdns_resolve_request_set_query_type(httpdns_resolve_request_t *request, const char *query_type) {
    httpdns_set_string_field(request, query_type, query_type);
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
    if (NULL == request) {
        return;
    }
    request->using_multi = using_multi;
    if (NULL != request->cache_key) {
        httpdns_sds_free(request->cache_key);
        request->cache_key = NULL;
    }
}

void httpdns_resolve_request_set_using_cache(httpdns_resolve_request_t *request, bool using_cache) {
    if (NULL == request) {
        return;
    }
    request->using_cache = using_cache;
}

void httpdns_resolve_request_set_callback(httpdns_resolve_request_t *request,
                                          httpdns_complete_callback_func_t callback,
                                          void *user_callback_param) {
    if (NULL == request) {
        return;
    }
    request->complete_callback_func = callback;
    request->user_callback_param = user_callback_param;
}


void httpdns_resolve_request_free(httpdns_resolve_request_t *request) {
    if (NULL == request) {
        return;
    }
    if (NULL != request->host) {
        httpdns_sds_free(request->host);
    }
    if (NULL != request->account_id) {
        httpdns_sds_free(request->account_id);
    }
    if (NULL != request->secret_key) {
        httpdns_sds_free(request->secret_key);
    }
    if (NULL != request->resolver) {
        httpdns_sds_free(request->resolver);
    }
    if (NULL != request->query_type) {
        httpdns_sds_free(request->query_type);
    }
    if (NULL != request->client_ip) {
        httpdns_sds_free(request->client_ip);
    }
    if (NULL != request->sdk_version) {
        httpdns_sds_free(request->sdk_version);
    }
    if (NULL != request->user_agent) {
        httpdns_sds_free(request->user_agent);
    }
    if (NULL != request->sdns_params) {
        httpdns_sds_free(request->sdns_params);
    }
    if (NULL != request->cache_key) {
        httpdns_sds_free(request->cache_key);
    }
    free(request);
}

