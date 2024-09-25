//
// Created by caogaoshuai on 2024/1/18.
//
#include <cjson/cJSON.h>
#include "hdns_log.h"
#include "hdns_sign.h"
#include "hdns_http.h"
#include "hdns_buf.h"

#include "hdns_resolver.h"

#define  HDNS_API_D                   "/d"
#define  HDNS_API_SIGN_D              "/sign_d"
#define  HDNS_API_RESOLVE             "/resolve"
#define  HDNS_API_SIGN_RESOLVE        "/sign_resolve"

#define  HDNS_QUERY_TYPE_A        "4"
#define  HDNS_QUERY_TYPE_AAAA     "6"
#define  HDNS_QUERY_TYPE_BOTH   "4,6"

static const char *hdns_query_type_to_string(hdns_query_type_t query_type_e) {
    switch (query_type_e) {
        case HDNS_QUERY_IPV4:
            return HDNS_QUERY_TYPE_A;
        case HDNS_QUERY_IPV6:
            return HDNS_QUERY_TYPE_AAAA;
        default:
            return HDNS_QUERY_TYPE_BOTH;
    }
}


static int parse_single_resv_resp(const char *body, hdns_list_head_t *resv_resps, const char *cache_key);

static int parse_multi_resv_resp(const char *body, hdns_list_head_t *resv_resps);

static void parse_ip_array(cJSON *c_json_array, hdns_list_head_t *ips);

static char *decode_html(hdns_pool_t *pool, char *src);

static void parse_resv_resp_from_json(hdns_list_head_t *resv_resps, cJSON *c_json_body, const char *cache_key);

static void parse_ip_array(cJSON *c_json_array, hdns_list_head_t *ips) {
    size_t array_size = cJSON_GetArraySize(c_json_array);
    if (array_size == 0) {
        return;
    }
    for (int i = 0; i < array_size; i++) {
        cJSON *ip_json = cJSON_GetArrayItem(c_json_array, i);
        hdns_list_add(ips, ip_json->valuestring, hdns_to_list_clone_fn_t(apr_pstrdup));
    }
}

static char *decode_html(hdns_pool_t *pool, char *src) {
    size_t length = strlen(src);
    char *dst = hdns_palloc(pool, length + 1);
    size_t src_index = 0;
    size_t dst_index = 0;

    char *escape_str[] = {"&amp;", "&lt;", "&gt;", "&quot;", "&apos;",};
    char unescape_ch[] = {'&', '<', '>', '\"', '\''};
    int escape_tb_len = sizeof(escape_str) / sizeof(escape_str[0]);

    while (src_index < length) {
        if (src[src_index] != '&') {
            dst[dst_index++] = src[src_index++];
            continue;
        }
        for (int i = 0; i < escape_tb_len; i++) {
            if (src_index + strlen(escape_str[i]) - 1 < length
                && strncmp(src + src_index, escape_str[i], strlen(escape_str[i])) == 0) {
                src_index += strlen(escape_str[i]);
                dst[dst_index++] = unescape_ch[i];
            }
        }
        dst[dst_index++] = src[src_index++];
    }
    dst[dst_index] = '\0';
    return dst;
}

static APR_INLINE void fill_ips_and_append_resps(hdns_resv_resp_t *resv_resp,
                                                 cJSON *c_json_body,
                                                 const char *ips_field,
                                                 hdns_list_head_t *resv_resps) {
    cJSON *ips_json = cJSON_GetObjectItem(c_json_body, ips_field);
    parse_ip_array(ips_json, resv_resp->ips);
    hdns_list_shuffle(resv_resp->ips);
    hdns_list_add(resv_resps, resv_resp, NULL);
}

static void parse_resv_resp_from_json(hdns_list_head_t *resv_resps, cJSON *c_json_body, const char *cache_key) {
    if (NULL == c_json_body) {
        return;
    }
    hdns_resv_resp_t *resv_resp = hdns_resv_resp_create_empty(resv_resps->pool, NULL, HDNS_RR_TYPE_A);
    cJSON *host_json = cJSON_GetObjectItem(c_json_body, "host");
    if (NULL != host_json) {
        resv_resp->host = apr_pstrdup(resv_resp->pool, host_json->valuestring);
    }
    cJSON *client_ip_json = cJSON_GetObjectItem(c_json_body, "client_ip");
    if (NULL != client_ip_json) {
        resv_resp->client_ip = apr_pstrdup(resv_resp->pool, client_ip_json->valuestring);
    }
    cJSON *ttl_json = cJSON_GetObjectItem(c_json_body, "ttl");
    if (NULL != ttl_json) {
        resv_resp->ttl = ttl_json->valueint;
    }
    cJSON *origin_ttl_json = cJSON_GetObjectItem(c_json_body, "origin_ttl");
    if (NULL != origin_ttl_json) {
        resv_resp->origin_ttl = origin_ttl_json->valueint;
    }
    cJSON *extra_json = cJSON_GetObjectItem(c_json_body, "extra");
    if (NULL != extra_json) {
        resv_resp->extra = decode_html(resv_resp->pool, extra_json->valuestring);
    }
    if (hdns_str_is_not_blank(cache_key)) {
        resv_resp->cache_key = apr_pstrdup(resv_resp->pool, cache_key);
    } else {
        resv_resp->cache_key = apr_pstrdup(resv_resp->pool, resv_resp->host);
    }
    resv_resp->query_time = apr_time_now();

    bool ips_exist = (cJSON_GetObjectItem(c_json_body, "ips") != NULL);
    bool ipsv6_exist = (cJSON_GetObjectItem(c_json_body, "ipsv6") != NULL);
    if (ips_exist && ipsv6_exist) {
        hdns_resv_resp_t *another_resp = hdns_resv_resp_clone(resv_resps->pool, resv_resp);
        fill_ips_and_append_resps(resv_resp, c_json_body, "ips", resv_resps);
        resv_resp->type = HDNS_RR_TYPE_A;
        fill_ips_and_append_resps(another_resp, c_json_body, "ipsv6", resv_resps);
        another_resp->type = HDNS_RR_TYPE_AAAA;
        return;
    }
    if (ips_exist) {
        fill_ips_and_append_resps(resv_resp, c_json_body, "ips", resv_resps);
        resv_resp->type = HDNS_RR_TYPE_A;
        cJSON *type_json = cJSON_GetObjectItem(c_json_body, "type");
        if (NULL != type_json) {
            resv_resp->type = type_json->valueint;
        }
        return;
    }
    if (ipsv6_exist) {
        fill_ips_and_append_resps(resv_resp, c_json_body, "ipsv6", resv_resps);
        resv_resp->type = HDNS_RR_TYPE_AAAA;
        return;
    }
}

static int parse_single_resv_resp(const char *body, hdns_list_head_t *resv_resps, const char *cache_key) {
    if (hdns_str_is_blank(body)) {
        hdns_log_info("parse single resolve failed, body is empty");
        return HDNS_ERROR;
    }
    cJSON *c_json_body = cJSON_Parse(body);
    if (NULL == c_json_body) {
        hdns_log_info("parse single resolve failed, body may be not json");
        return HDNS_ERROR;
    }
    parse_resv_resp_from_json(resv_resps, c_json_body, cache_key);
    cJSON_Delete(c_json_body);
    return HDNS_OK;
}

static int parse_multi_resv_resp(const char *body, hdns_list_head_t *resv_resps) {
    if (hdns_str_is_blank(body)) {
        hdns_log_info("parse multi resolve failed, body is empty");
        return HDNS_ERROR;
    }
    cJSON *c_json_body = cJSON_Parse(body);
    if (NULL == c_json_body) {
        hdns_log_info("parse multi resolve failed, body may be not json");
        return HDNS_ERROR;
    }
    cJSON *dns_json = cJSON_GetObjectItem(c_json_body, "dns");
    if (NULL != dns_json) {
        int dns_size = cJSON_GetArraySize(dns_json);
        for (int i = 0; i < dns_size; i++) {
            cJSON *single_resp_json = cJSON_GetArrayItem(dns_json, i);
            parse_resv_resp_from_json(resv_resps, single_resp_json, NULL);
        }
    } else {
        hdns_log_info("parse multi resolve failed, body is %s", body);
    }
    cJSON_Delete(c_json_body);
    return HDNS_OK;
}


hdns_http_response_t *hdns_resv_send_req(hdns_pool_t *req_pool, hdns_resv_req_t *resv_req) {
    hdns_http_request_t *http_req = hdns_http_request_create(req_pool);
    http_req->proto = resv_req->using_https ? HDNS_HTTPS_PREFIX : HDNS_HTTP_PREFIX;
    http_req->host = apr_pstrdup(req_pool, resv_req->resolver);

    const bool using_sign = (resv_req->using_sign && NULL != resv_req->secret_key);
    const char *http_api = resv_req->using_multi ? (using_sign ? HDNS_API_SIGN_RESOLVE : HDNS_API_RESOLVE)
                                                 : (using_sign ? HDNS_API_SIGN_D : HDNS_API_D);

    http_req->uri = apr_pstrcat(req_pool, "/", resv_req->account_id, http_api, NULL);
    apr_table_set(http_req->query_params, "host", resv_req->host);
    apr_table_set(http_req->query_params, "query", hdns_query_type_to_string(resv_req->query_type));

    apr_table_set(http_req->query_params, "platform", HDNS_PLATFORM);
    apr_table_set(http_req->query_params, "sdk_version", HDNS_VER);
    apr_table_set(http_req->query_params, "sid", resv_req->session_id);
    if (using_sign) {
        hdns_sign_t *signature = hdns_gen_resv_req_sign(req_pool, resv_req->host, resv_req->secret_key);
        apr_table_set(http_req->query_params, "s", signature->sign);
        apr_table_set(http_req->query_params, "t", signature->timestamp);
    }
    if (hdns_str_is_not_blank(resv_req->client_ip)) {
        apr_table_set(http_req->query_params, "ip", resv_req->client_ip);
    }
    if (!hdns_is_empty_table(resv_req->sdns_params)) {
        apr_table_overlap(http_req->query_params, resv_req->sdns_params, APR_OVERLAP_TABLES_SET);
    }

    hdns_http_controller_t *http_ctl = hdns_http_controller_create(req_pool);
    http_ctl->timeout = resv_req->timeout_ms;
    hdns_http_response_t *http_resp = hdns_http_response_create(req_pool);
    hdns_http_send_request(http_ctl, http_req, http_resp);
    return http_resp;
}

void hdns_parse_resv_resp(hdns_resv_req_t *resv_req,
                          hdns_http_response_t *http_resp,
                          hdns_pool_t *pool,
                          hdns_list_head_t *resv_resps) {
    char *body = hdns_buf_list_content(pool, http_resp->body);
    if (resv_req->using_multi) {
        parse_multi_resv_resp(body, resv_resps);
    } else {
        parse_single_resv_resp(body, resv_resps, resv_req->cache_key);
    }
}

hdns_resv_req_t *hdns_resv_req_new(hdns_pool_t *pool, hdns_config_t *config) {
    if (NULL == pool) {
        hdns_pool_create(&pool, NULL);
    }
    hdns_resv_req_t *resv_req = hdns_palloc(pool, sizeof(hdns_resv_req_t));
    resv_req->pool = pool;
    resv_req->host = NULL;
    apr_thread_mutex_lock(config->lock);
    resv_req->account_id = apr_pstrdup(pool, config->account_id);
    resv_req->secret_key = apr_pstrdup(pool, config->secret_key);
    resv_req->using_cache = config->using_cache;
    resv_req->using_https = config->using_https;
    resv_req->using_sign = hdns_str_is_not_blank(config->secret_key) && config->using_sign;
    resv_req->timeout_ms = config->timeout;
    resv_req->retry_times = config->retry_times;
    apr_thread_mutex_unlock(config->lock);
    resv_req->resolver = NULL;
    resv_req->session_id = apr_pstrdup(pool, config->session_id);
    resv_req->query_type = HDNS_QUERY_AUTO;
    resv_req->client_ip = NULL;
    resv_req->user_agent = NULL;
    resv_req->sdns_params = hdns_table_make(pool, 5);
    resv_req->using_multi = false;
    resv_req->cache_key = NULL;
    resv_req->resv_resp_callback = NULL;
    resv_req->resv_resp_cb_param = NULL;
    apr_thread_mutex_create(&resv_req->lock, APR_THREAD_MUTEX_DEFAULT, pool);
    return resv_req;
}

hdns_resv_resp_t *hdns_resv_resp_clone(hdns_pool_t *pool, const hdns_resv_resp_t *origin_resp) {
    if (NULL == pool) {
        hdns_pool_create(&pool, NULL);
    }
    hdns_resv_resp_t *new_resp = hdns_palloc(pool, sizeof(hdns_resv_resp_t));
    new_resp->pool = pool;
    new_resp->host = apr_pstrdup(pool, origin_resp->host);
    new_resp->client_ip = apr_pstrdup(pool, origin_resp->client_ip);
    new_resp->extra = apr_pstrdup(pool, origin_resp->extra);
    new_resp->ips = hdns_list_new(pool);
    hdns_list_dup(new_resp->ips, origin_resp->ips, hdns_to_list_clone_fn_t(apr_pstrdup));
    new_resp->query_time = origin_resp->query_time;
    new_resp->ttl = origin_resp->ttl;
    new_resp->origin_ttl = origin_resp->origin_ttl;
    new_resp->cache_key = apr_pstrdup(pool, origin_resp->cache_key);
    new_resp->type = origin_resp->type;
    new_resp->from_localdns = origin_resp->from_localdns;
    return new_resp;
}

void hdns_resv_resp_destroy(hdns_resv_resp_t *resp) {
    if (resp != NULL) {
        hdns_pool_destroy(resp->pool);
    }
}

hdns_status_t hdns_resv_req_valid(const hdns_resv_req_t *req) {
    if (NULL == req) {
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "req is null",
                                 NULL);
    }
    apr_thread_mutex_lock(req->lock);
    if (hdns_str_is_blank(req->account_id)) {
        apr_thread_mutex_unlock(req->lock);
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "account_id is blank",
                                 req->session_id);
    }
    if (req->using_sign && hdns_str_is_blank(req->secret_key)) {
        apr_thread_mutex_unlock(req->lock);
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "using sign but secret_key is blank",
                                 req->session_id);
    }
    if (hdns_str_is_blank(req->host)) {
        apr_thread_mutex_unlock(req->lock);
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "host is blank",
                                 req->session_id);
    }
    if (req->query_type > HDNS_QUERY_BOTH) {
        apr_thread_mutex_unlock(req->lock);
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "query_type is invalid",
                                 req->session_id);
    }
    apr_thread_mutex_unlock(req->lock);
    return hdns_status_ok(req->session_id);
}


hdns_resv_resp_t *hdns_resv_resp_create_empty(hdns_pool_t *pool, const char *host, hdns_rr_type_t type) {
    if (pool == NULL) {
        hdns_pool_create(&pool, NULL);
    }
    hdns_resv_resp_t *resv_resp = hdns_palloc(pool, sizeof(hdns_resv_resp_t));
    resv_resp->pool = pool;
    resv_resp->host = apr_pstrdup(pool, host);
    resv_resp->ips = hdns_list_new(pool);
    resv_resp->client_ip = NULL;
    resv_resp->extra = NULL;
    resv_resp->type = type;
    resv_resp->origin_ttl = 60;
    resv_resp->ttl = 60;
    resv_resp->query_time = apr_time_now();
    resv_resp->cache_key = NULL;
    resv_resp->from_localdns = false;
    return resv_resp;
}

char *hdns_resv_resp_to_str(hdns_pool_t *pool, hdns_resv_resp_t *resp) {
    if (pool == NULL || resp == NULL) {
        return "hdns_resv_resp_t()";
    }
    hdns_list_head_t *ips = hdns_list_new(pool);
    if (hdns_list_is_empty(resp->ips)) {
        char *bracket = "[]";
        hdns_buf_t *buf = hdns_create_buf(pool, strlen(bracket));
        hdns_buf_append_string(pool, buf, bracket, (int) strlen(bracket));
        hdns_list_add(ips, buf, NULL);
    } else {
        hdns_list_for_each_entry_safe(cursor, resp->ips) {
            int str_len = (int) strlen(cursor->data);
            hdns_buf_t *buf = hdns_create_buf(pool, str_len);

            if (cursor->prev == resp->ips) {
                hdns_buf_append_string(pool, buf, "[", (int) strlen("["));
            }
            hdns_buf_append_string(pool, buf, cursor->data, str_len);
            if (cursor->next != resp->ips) {
                hdns_buf_append_string(pool, buf, ",", (int) strlen(","));
            } else {
                hdns_buf_append_string(pool, buf, "]", (int) strlen("]"));
            }
            hdns_list_add(ips, buf, NULL);
        }
    }

    return apr_pstrcat(pool,
                       "hdns_resv_resp_t("
                       "host=",
                       hdns_str_is_not_blank(resp->host) ? resp->host : "",
                       ","
                       "client_ip=",
                       hdns_str_is_not_blank(resp->client_ip) ? resp->client_ip : "",
                       ",",
                       "type=",
                       resp->type == HDNS_RR_TYPE_A ? "A" : "AAAA",
                       ","
                       "extra=",
                       hdns_str_is_not_blank(resp->extra) ? resp->extra : "",
                       ","
                       "cache_key=",
                       hdns_str_is_not_blank(resp->cache_key) ? resp->cache_key : "",
                       ",",
                       "ttl=",
                       apr_itoa(pool, resp->ttl),
                       ",",
                       "orgin_ttl=",
                       apr_itoa(pool, resp->origin_ttl),
                       ",",
                       "ips=",
                       hdns_buf_list_content(pool, ips),
                       ",",
                       "from_localdns=",
                       resp->from_localdns ? "true" : "false",
                       ")",
                       NULL);
}

void hdns_resv_req_free(hdns_resv_req_t *req) {
    if (req != NULL) {
        apr_thread_mutex_destroy(req->lock);
        hdns_pool_destroy(req->pool);
    }
}

hdns_resv_req_t *hdns_resv_req_clone(hdns_pool_t *pool, const hdns_resv_req_t *origin_req) {
    if (NULL == pool) {
        hdns_pool_create(&pool, NULL);
    }
    hdns_resv_req_t *resv_req = hdns_palloc(pool, sizeof(hdns_resv_req_t));
    resv_req->pool = pool;
    apr_thread_mutex_lock(origin_req->lock);
    resv_req->host = apr_pstrdup(pool, origin_req->host);
    resv_req->account_id = apr_pstrdup(pool, origin_req->account_id);
    resv_req->secret_key = apr_pstrdup(pool, origin_req->secret_key);
    resv_req->using_cache = origin_req->using_cache;
    resv_req->using_https = origin_req->using_https;
    resv_req->using_sign = origin_req->using_sign;
    resv_req->timeout_ms = origin_req->timeout_ms;
    resv_req->retry_times = origin_req->retry_times;
    resv_req->session_id = apr_pstrdup(pool, origin_req->session_id);
    resv_req->resolver = apr_pstrdup(pool, origin_req->resolver);
    resv_req->query_type = origin_req->query_type;
    resv_req->client_ip = apr_pstrdup(pool, origin_req->client_ip);
    resv_req->user_agent = apr_pstrdup(pool, origin_req->user_agent);
    resv_req->sdns_params = apr_table_copy(pool, origin_req->sdns_params);
    resv_req->using_multi = origin_req->using_multi;
    resv_req->cache_key = apr_pstrdup(pool, origin_req->cache_key);;
    resv_req->resv_resp_callback = origin_req->resv_resp_callback;
    resv_req->resv_resp_cb_param = origin_req->resv_resp_cb_param;
    apr_thread_mutex_unlock(origin_req->lock);
    apr_thread_mutex_create(&resv_req->lock, APR_THREAD_MUTEX_DEFAULT, pool);
    return resv_req;
}
