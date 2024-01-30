//
// Created by cagaoshuai on 2024/1/29.
//
#include "httpdns_resolve_request.h"
#include "httpdns_error_type.h"
#include "log.h"
#include "sds.h"
#include "httpdns_memory.h"



int32_t httpdns_resolve_request_valid(httpdns_resolve_request_t *request) {
    if (NULL == request) {
        log_info("resolve request valid failed, request is NULL");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (IS_BLANK_STRING(request->host)) {
        log_info("resolve request valid failed, host is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (IS_BLANK_STRING(request->account_id)) {
        log_info("resolve request valid failed, account_id is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (IS_BLANK_STRING(request->resolver)) {
        log_info("resolve request valid failed, resolver is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (IS_BLANK_STRING(request->query_type)) {
        log_info("resolve request valid failed, query_type is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (IS_BLANK_STRING(request->user_agent)) {
        log_info("resolve request valid failed, user_agent is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (IS_BLANK_STRING(request->sdk_version)) {
        log_info("resolve request valid failed, sdk_version is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (request->using_multi && IS_NOT_BLANK_STRING(request->cache_key)) {
        log_info(
                "resolve request valid failed, when using httpdns api resolve or sign_resolve, cache_key must be blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (request->timeout_ms <= 0) {
        log_info("resolve request valid failed, timeout_ms is less than 0");
        return HTTPDNS_PARAMETER_ERROR;
    }
    return HTTPDNS_SUCCESS;
}



httpdns_resolve_request_t *
httpdns_resolve_request_create(httpdns_config_t *config, char *host, char *resolver, char *query_type) {
    if (HTTPDNS_SUCCESS != httpdns_config_valid(config) || NULL == host) {
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


sds httpdns_resolve_request_to_string(httpdns_resolve_request_t *request) {
    if (NULL == request) {
        return sdsnew("httpdns_resolve_request_t()");
    }
    sds dst_str = sdsnew("httpdns_resolve_request_t(host=");
    SDS_CAT(dst_str, request->host);
    SDS_CAT(dst_str, ",account_id=");
    SDS_CAT(dst_str, request->account_id);
    SDS_CAT(dst_str, ",secret_key=");
    SDS_CAT(dst_str, request->secret_key);
    SDS_CAT(dst_str, ",resolver=");
    SDS_CAT(dst_str, request->resolver);
    SDS_CAT(dst_str, ",query_type=");
    SDS_CAT(dst_str, request->query_type);
    SDS_CAT(dst_str, ",client_ip=");
    SDS_CAT(dst_str, request->client_ip);

    SDS_CAT(dst_str, ",sdk_version=");
    SDS_CAT(dst_str, request->sdk_version);
    SDS_CAT(dst_str, ",user_agent=");
    SDS_CAT(dst_str, request->user_agent);
    SDS_CAT(dst_str, ",sdns_params=");
    SDS_CAT(dst_str, request->sdns_params);
    SDS_CAT(dst_str, ",cache_key=");
    SDS_CAT(dst_str, request->cache_key);

    SDS_CAT(dst_str, ",using_https=");
    SDS_CAT_INT(dst_str, request->using_https)
    SDS_CAT(dst_str, ",using_sign=");
    SDS_CAT_INT(dst_str, request->using_sign)
    SDS_CAT(dst_str, ",using_multi=");
    SDS_CAT_INT(dst_str, request->using_multi)
    SDS_CAT(dst_str, ",using_cache=");
    SDS_CAT_INT(dst_str, request->using_cache)
    SDS_CAT(dst_str, ",timeout_ms=");
    SDS_CAT_INT(dst_str, request->timeout_ms)
    SDS_CAT(dst_str, ")");
    return dst_str;
}

httpdns_resolve_request_t *httpdns_resolve_request_clone(httpdns_resolve_request_t *origin_resolve_request) {
    if (NULL == origin_resolve_request) {
        log_info("resolve request clone failed, origin resolve reqeust is NULL");
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
        log_info("append sdns param failed, request or key or value is NULL");
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
    if (NULL == request) {
        return;
    }
    request->using_multi = using_multi;
    if (NULL != request->cache_key) {
        sdsfree(request->cache_key);
        request->cache_key = NULL;
    }
}


void httpdns_resolve_request_free(httpdns_resolve_request_t *request) {
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

