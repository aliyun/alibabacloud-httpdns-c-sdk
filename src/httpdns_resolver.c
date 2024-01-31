//
// Created by cagaoshuai on 2024/1/18.
//

#include "httpdns_resolver.h"
#include "httpdns_sign.h"
#include "http_response_parser.h"
#include "httpdns_memory.h"
#include "httpdns_time.h"
#include "httpdns_ip.h"
#include "log.h"
#include "httpdns_resolve_result.h"
#include "httpdns_string.h"

int32_t httpdns_resolver_single_resolve(httpdns_resolve_param_t *resolve_param) {
    if (NULL == resolve_param || NULL == resolve_param->request) {
        log_info("single resolve failed, resolve_param or request is NULL");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    httpdns_resolve_request_t *request = resolve_param->request;
    if (httpdns_resolve_request_valid(request) != HTTPDNS_SUCCESS) {
        log_info("single resolve failed, httpdns resolve request is invalid");
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
        log_info("multi resolve failed, resolve_params is NULL");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    size_t resolve_params_size = httpdns_list_size(resolve_params);
    if (resolve_params_size <= 0) {
        log_info("multi resolve failed, resolve_params is empty");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    log_debug("multi resolve params size %d", resolve_params_size);
    size_t http_context_size = 0;
    NEW_EMPTY_LIST_IN_STACK(http_contexts);
    httpdns_list_for_each_entry(param_cursor, resolve_params) {
        httpdns_resolve_param_t *resolve_param = param_cursor->data;
        httpdns_resolve_request_t *request = resolve_param->request;
        if (HTTPDNS_SUCCESS != httpdns_resolve_request_valid(request)) {
            log_info("httpdns resolve request is invalid, skip");
            continue;
        }

        sds request_str = httpdns_resolve_request_to_string(request);
        log_debug("multi resolve request %s", request_str);
        sdsfree(request_str);

        const char *http_scheme = request->using_https ? HTTPS_SCHEME : HTTP_SCHEME;
        const bool using_sign = (request->using_sign && NULL != request->secret_key);
        const char *http_api = request->using_multi ? (using_sign ? HTTPDNS_API_SIGN_RESOLVE : HTTPDNS_API_RESOLVE)
                                                    : (using_sign ? HTTPDNS_API_SIGN_D : HTTPDNS_API_D);
        sds url = sdsnew(http_scheme);
        SDS_CAT(url, request->resolver);
        SDS_CAT(url, "/");
        SDS_CAT(url, request->account_id);
        SDS_CAT(url, http_api);
        SDS_CAT(url, "?host=");
        SDS_CAT(url, request->host);
        SDS_CAT(url, "&query=");
        SDS_CAT(url, request->query_type);
        if (using_sign) {
            httpdns_signature_t *signature = httpdns_signature_new(request->host,
                                                                   request->secret_key,
                                                                   MAX_RESOLVE_SIGNATURE_OFFSET_TIME,
                                                                   httpdns_time_now());
            SDS_CAT(url, "&s=");
            SDS_CAT(url, signature->sign);
            SDS_CAT(url, "&t=");
            SDS_CAT(url, signature->timestamp);
            httpdns_signature_free(signature);
        }
        if (IS_NOT_BLANK_STRING(request->client_ip)) {
            SDS_CAT(url, "&ip=");
            SDS_CAT(url, request->client_ip);
        }
        SDS_CAT(url, "&platform=linux&sdk_version=");
        SDS_CAT(url, request->sdk_version);

        httpdns_http_context_t *http_context = httpdns_http_context_new(url, request->timeout_ms);
        httpdns_http_context_set_user_agent(http_context, request->user_agent);
        httpdns_http_context_set_private_data(http_context, resolve_param);
        httpdns_list_add(&http_contexts, http_context, NULL);

        log_debug("multi resolve url %s", url);
        sdsfree(url);

        http_context_size++;
    }

    httpdns_http_multiple_exchange(&http_contexts);

    log_debug("http context size is %d", http_context_size);
    httpdns_list_for_each_entry(http_context_cursor, &http_contexts) {
        httpdns_http_context_t *http_context = http_context_cursor->data;
        httpdns_resolve_param_t *resolve_param = http_context->private_data;
        if (NULL != resolve_param->http_finish_callback_func) {
            resolve_param->http_finish_callback_func(
                    http_context->response_body,
                    http_context->response_status,
                    http_context->response_rt_ms,
                    resolve_param->user_http_finish_callback_param);
        }
    }
    httpdns_list_free(&http_contexts, DATA_FREE_FUNC(httpdns_http_context_free));
    return HTTPDNS_SUCCESS;
}

httpdns_resolve_context_t *httpdns_resolve_context_new(httpdns_resolve_request_t *request) {
    if (NULL == request) {
        log_info("create resolve context failed, request is NULL");
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(resolve_context, httpdns_resolve_context_t);
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
    httpdns_list_free(&resolve_context->result, DATA_FREE_FUNC(httpdns_resolve_result_free));
    free(resolve_context);
}

httpdns_resolve_param_t *httpdns_resolve_param_new(httpdns_resolve_request_t *request) {
    if (NULL == request) {
        log_info("create resolve param failed, request is NULL");
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(resolve_param, httpdns_resolve_param_t);
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
        resolve_param->callback_param_free_func(resolve_param->user_http_finish_callback_param);
    }
    free(resolve_param);
}