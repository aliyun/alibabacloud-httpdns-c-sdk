//
// Created by caogaoshuai on 2024/1/18.
//

#include "httpdns_log.h"
#include "httpdns_memory.h"
#include "httpdns_resolve_result.h"
#include "http_response_parser.h"
#include "httpdns_sds.h"
#include "httpdns_sign.h"
#include "httpdns_time.h"

#include "httpdns_resolver.h"

int32_t httpdns_resolver_single_resolve(httpdns_resolve_param_t *resolve_param) {
    if (NULL == resolve_param || NULL == resolve_param->request) {
        httpdns_log_info("single resolve failed, resolve_param or request is NULL");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    httpdns_resolve_request_t *request = resolve_param->request;
    if (httpdns_resolve_request_valid(request) != HTTPDNS_SUCCESS) {
        httpdns_log_info("single resolve failed, httpdns resolve request is invalid");
        return HTTPDNS_PARAMETER_ERROR;
    }
    httpdns_list_new_empty_in_stack(resolve_params);
    httpdns_list_add(&resolve_params, resolve_param, NULL);
    int32_t ret = httpdns_resolver_multi_resolve(&resolve_params);
    httpdns_list_free(&resolve_params, NULL);
    return ret;
}

static bool is_valid_ipv6(const char *ipv6) {
    struct in6_addr addr6;
    return inet_pton(AF_INET6, ipv6, &addr6) == 1;
}

int32_t httpdns_resolver_multi_resolve(httpdns_list_head_t *resolve_params) {
    if (NULL == resolve_params) {
        httpdns_log_info("multi resolve failed, resolve_params is NULL");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    size_t resolve_params_size = httpdns_list_size(resolve_params);
    if (resolve_params_size <= 0) {
        httpdns_log_info("multi resolve failed, resolve_params is empty");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    httpdns_log_debug("multi resolve params size %d", resolve_params_size);
    httpdns_list_new_empty_in_stack(http_contexts);
    size_t http_context_size = 0;
    httpdns_list_for_each_entry(param_cursor, resolve_params) {
        httpdns_resolve_param_t *resolve_param = param_cursor->data;
        httpdns_resolve_request_t *request = resolve_param->request;

        httpdns_sds_t request_str = httpdns_resolve_request_to_string(request);
        httpdns_log_debug("multi resolve request %s", request_str);
        httpdns_sds_free(request_str);

        const char *http_scheme = request->using_https ? HTTPDNS_HTTPS_SCHEME : HTTPDNS_HTTP_SCHEME;
        const bool using_sign = (request->using_sign && NULL != request->secret_key);
        const char *http_api = request->using_multi ? (using_sign ? HTTPDNS_API_SIGN_RESOLVE : HTTPDNS_API_RESOLVE)
                                                    : (using_sign ? HTTPDNS_API_SIGN_D : HTTPDNS_API_D);
        httpdns_sds_t url = httpdns_sds_new(http_scheme);
        if (is_valid_ipv6(request->resolver)) {
            httpdns_sds_cat_easily(url, "[");
        }
        httpdns_sds_cat_easily(url, request->resolver);
        if (is_valid_ipv6(request->resolver)) {
            httpdns_sds_cat_easily(url, "]");
        }
        httpdns_sds_cat_easily(url, "/");
        httpdns_sds_cat_easily(url, request->account_id);
        httpdns_sds_cat_easily(url, http_api);
        httpdns_sds_cat_easily(url, "?host=");
        httpdns_sds_cat_easily(url, request->host);
        httpdns_sds_cat_easily(url, "&query=");
        httpdns_sds_cat_easily(url, request->query_type);
        if (using_sign) {
            httpdns_signature_t *signature = httpdns_signature_new(request->host,
                                                                   request->secret_key,
                                                                   HTTPDNS_MAX_RESOLVE_SIGNATURE_OFFSET_TIME,
                                                                   httpdns_time_now());
            httpdns_sds_cat_easily(url, "&s=");
            httpdns_sds_cat_easily(url, signature->sign);
            httpdns_sds_cat_easily(url, "&t=");
            httpdns_sds_cat_easily(url, signature->timestamp);
            httpdns_signature_free(signature);
        }
        if (httpdns_string_is_not_blank(request->client_ip)) {
            httpdns_sds_cat_easily(url, "&ip=");
            httpdns_sds_cat_easily(url, request->client_ip);
        }
        if (httpdns_string_is_not_blank(request->sdns_params)) {
            httpdns_sds_cat_easily(url, request->sdns_params);
        }
        httpdns_sds_cat_easily(url, "&platform=linux&sdk_version=");
        httpdns_sds_cat_easily(url, request->sdk_version);

        httpdns_http_context_t *http_context = httpdns_http_context_new(url, request->timeout_ms);
        httpdns_http_context_set_user_agent(http_context, request->user_agent);
        httpdns_http_context_set_private_data(http_context, resolve_param);
        httpdns_list_add(&http_contexts, http_context, NULL);

        httpdns_log_debug("multi resolve url %s", url);
        httpdns_sds_free(url);

        http_context_size++;
    }

    httpdns_http_multiple_exchange(&http_contexts);

    httpdns_log_debug("http context size is %d", http_context_size);
    httpdns_list_for_each_entry(http_context_cursor, &http_contexts) {
        httpdns_http_context_t *http_context = http_context_cursor->data;
        httpdns_resolve_param_t *resolve_param = http_context->private_data;
        if (NULL != resolve_param->http_complete_callback_func) {
            resolve_param->http_complete_callback_func(
                    http_context,
                    resolve_param->user_http_complete_callback_param);
        }
    }
    httpdns_list_free(&http_contexts, to_httpdns_data_free_func(httpdns_http_context_free));
    return HTTPDNS_SUCCESS;
}

httpdns_resolve_context_t *httpdns_resolve_context_new(const httpdns_resolve_request_t *request) {
    if (NULL == request) {
        httpdns_log_info("create resolve context failed, request is NULL");
        return NULL;
    }
    httpdns_new_object_in_heap(resolve_context, httpdns_resolve_context_t);
    resolve_context->request = httpdns_resolve_request_clone(request);
    httpdns_list_init(&resolve_context->result);
    return resolve_context;
}

void httpdns_resolve_context_free(httpdns_resolve_context_t *resolve_context) {
    if (NULL == resolve_context) {
        return;
    }
    if (NULL != resolve_context->request) {
        httpdns_resolve_request_free(resolve_context->request);
    }
    httpdns_list_free(&resolve_context->result, to_httpdns_data_free_func(httpdns_resolve_result_free));
    free(resolve_context);
}

httpdns_resolve_param_t *httpdns_resolve_param_new(httpdns_resolve_request_t *request) {
    if (NULL == request) {
        httpdns_log_info("create resolve param failed, request is NULL");
        return NULL;
    }
    httpdns_new_object_in_heap(resolve_param, httpdns_resolve_param_t);
    resolve_param->request = httpdns_resolve_request_clone(request);
    return resolve_param;
}

void httpdns_resolve_param_free(httpdns_resolve_param_t *resolve_param) {
    if (NULL == resolve_param) {
        return;
    }
    if (NULL != resolve_param->request) {
        httpdns_resolve_request_free(resolve_param->request);
    }
    if (NULL != resolve_param->callback_param_free_func) {
        resolve_param->callback_param_free_func(resolve_param->user_http_complete_callback_param);
    }
    free(resolve_param);
}