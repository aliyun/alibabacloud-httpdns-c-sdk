//
// Created by cagaoshuai on 2024/3/22.
//

#include <apr_thread_mutex.h>
#include <apr_file_io.h>

#include "hdns_log.h"
#include "hdns_define.h"
#include "hdns_string.h"
#include "hdns_fstack.h"
#include "hdns_buf.h"
#include "hdns_session.h"
#include "hdns_utils.h"

#include "hdns_transport.h"


static int hdns_curl_transport_setup(hdns_http_transport_t *t);

static void hdns_init_curl_headers(hdns_http_transport_t *t);

static void hdns_init_curl_sni(hdns_http_transport_t *t);

static int hdns_init_curl_url(hdns_http_transport_t *t);

static size_t hdns_curl_default_header_callback(char *buffer, size_t size, size_t nitems, void *userdata);

static size_t hdns_curl_default_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);

static size_t hdns_curl_default_read_callback(char *buffer, size_t size, size_t nitems, void *instream);

static int hdns_curl_debug_callback(void *handle, curl_infotype type, char *data, size_t size, void *userp);

static int hdns_url_encode(char *dest, const char *src, int maxSrcSize, bool slash);

static int hdns_query_params_to_string(hdns_pool_t *p, hdns_table_t *query_params, hdns_string_t *querystr);

static void hdns_curl_transport_headers_done(hdns_http_transport_t *t);

static int hdns_curl_code_to_status(CURLcode code);

static void hdns_curl_transport_finish(hdns_http_transport_t *t);

static void hdns_move_transport_state(hdns_http_transport_t *t, hdns_transport_state_e s);


static int hdns_curl_debug_callback(void *handle, curl_infotype type, char *data, size_t size, void *userp) {
    hdns_unused_var(userp);
    switch (type) {
        default:
            break;
        case CURLINFO_TEXT:
            hdns_log_debug("curl:%pp=> Info: %.*s", handle, (int) size, data);
            break;
        case CURLINFO_HEADER_OUT:
            hdns_log_debug("curl:%pp=> Send header: %.*s", handle, (int) size, data);
            break;
        case CURLINFO_HEADER_IN:
            hdns_log_debug("curl:%pp=> Recv header: %.*s", handle, (int) size, data);
            break;
    }
    return 0;
}

static void hdns_init_curl_sni(hdns_http_transport_t *t) {
    if (hdns_str_is_blank(t->controller->ca_host)) {
        return;
    }
    if (hdns_is_valid_ipv6(t->req->host) || hdns_is_valid_ipv4(t->req->host)) {
        char *sni = apr_pstrcat(t->pool, t->controller->ca_host, ":443:", t->req->host, NULL);
        t->curl_ctx->sni = curl_slist_append(t->curl_ctx->sni, sni);
        union hdns_func_u func;
        func.func1 = (hdns_func1_pt) curl_slist_free_all;
        hdns_fstack_push(t->cleanup, t->curl_ctx->sni, func, 1);
    }
}

static void hdns_init_curl_headers(hdns_http_transport_t *t) {
    int pos;
    char *header;
    const hdns_array_header_t *tarr;
    const hdns_table_entry_t *telts;
    union hdns_func_u func;

    if (t->req->method == HDNS_HTTP_PUT || t->req->method == HDNS_HTTP_POST) {
        header = apr_psprintf(t->pool, "Content-Length: %" APR_INT64_T_FMT, (apr_int64_t) t->req->body_len);
        t->curl_ctx->headers = curl_slist_append(t->curl_ctx->headers, header);
    }

    tarr = hdns_table_elts(t->req->headers);
    telts = (hdns_table_entry_t *) tarr->elts;
    for (pos = 0; pos < tarr->nelts; ++pos) {
        header = apr_psprintf(t->pool, "%s: %s", telts[pos].key, telts[pos].val);
        t->curl_ctx->headers = curl_slist_append(t->curl_ctx->headers, header);
    }

    func.func1 = (hdns_func1_pt) curl_slist_free_all;
    hdns_fstack_push(t->cleanup, t->curl_ctx->headers, func, 1);
}

int hdns_url_encode(char *dest, const char *src, int maxSrcSize, bool slash) {
    static const char *hex = "0123456789ABCDEF";

    int len = 0;
    unsigned char c;

    while ((c = *src++)) {
        if (++len > maxSrcSize) {
            *dest = 0;
            return HDNS_INVALID_ARGUMENT;
        }
        if (isalnum(c) || (c == '-') || (c == '_') || (c == '.') || (c == '~')) {
            *dest++ = hdns_to_char(c);
            continue;
        }
        if (c == ' ') {
            *dest++ = '%';
            *dest++ = '2';
            *dest++ = '0';
            continue;
        }
        if (c == '/' && slash) {
            *dest++ = hdns_to_char(c);
            continue;
        }
        *dest++ = '%';
        *dest++ = hex[c >> 4];
        *dest++ = hex[c & 15];
    }
    *dest = 0;

    return HDNS_OK;
}


int hdns_query_params_to_string(hdns_pool_t *p, hdns_table_t *query_params, hdns_string_t *querystr) {
    int pos;
    int len;
    char sep = '?';
    char ebuf[HDNS_MAX_QUERY_ARG_LEN * 3 + 1];
    char abuf[HDNS_MAX_QUERY_ARG_LEN * 6 + 128];
    int max_len;
    const hdns_array_header_t *tarr;
    const hdns_table_entry_t *telts;
    hdns_buf_t *querybuf;

    if (apr_is_empty_table(query_params)) {
        return HDNS_OK;
    }

    max_len = sizeof(abuf) - 1;
    querybuf = hdns_create_buf(p, 256);
    hdns_str_null(querystr);

    tarr = hdns_table_elts(query_params);
    telts = (hdns_table_entry_t *) tarr->elts;

    for (pos = 0; pos < tarr->nelts; ++pos) {
        if (hdns_url_encode(ebuf, telts[pos].key, HDNS_MAX_QUERY_ARG_LEN, false) != HDNS_OK) {
            hdns_log_error("query param args too big, key:%s.", telts[pos].key);
            return HDNS_INVALID_ARGUMENT;
        }
        len = apr_snprintf(abuf, max_len, "%c%s", sep, ebuf);
        if (telts[pos].val != NULL && *telts[pos].val != '\0') {
            if (hdns_url_encode(ebuf, telts[pos].val, HDNS_MAX_QUERY_ARG_LEN, false) != HDNS_OK) {
                hdns_log_error("query param args too big, value:%s.", telts[pos].val);
                return HDNS_INVALID_ARGUMENT;
            }
            len += apr_snprintf(abuf + len, max_len - len, "=%s", ebuf);
            if (len >= HDNS_MAX_QUERY_ARG_LEN) {
                hdns_log_error("query param args too big, %s.", abuf);
                return HDNS_INVALID_ARGUMENT;
            }
        }
        hdns_buf_append_string(p, querybuf, abuf, len);
        sep = '&';
    }

    // result
    querystr->data = (char *) querybuf->pos;
    querystr->len = hdns_buf_size(querybuf);

    return HDNS_OK;
}


static int hdns_init_curl_url(hdns_http_transport_t *t) {
    int rs;
    hdns_string_t querystr;
    char uristr[3 * HDNS_MAX_URI_LEN + 1];

    uristr[0] = '\0';
    hdns_str_null(&querystr);

    if ((rs = hdns_url_encode(uristr, t->req->uri, HDNS_MAX_URI_LEN, true)) != HDNS_OK) {
        t->resp->extra_info->error_code = rs;
        t->resp->extra_info->reason = "uri invalid argument.";
        return rs;
    }

    if ((rs = hdns_query_params_to_string(t->pool, t->req->query_params, &querystr)) != HDNS_OK) {
        t->resp->extra_info->error_code = rs;
        t->resp->extra_info->reason = "query param invalid argument.";
        return rs;
    }

    char *host = apr_pstrdup(t->pool, t->req->host);
    if (hdns_is_valid_ipv4(host) || hdns_is_valid_ipv6(host)) {
        if (hdns_str_start_with(t->req->proto, HDNS_HTTPS_PREFIX) && hdns_str_is_not_blank(t->controller->ca_host)) {
            host = apr_pstrdup(t->pool, t->controller->ca_host);
        }
        if (hdns_is_valid_ipv6(host)) {
            host = apr_pstrcat(t->pool, "[", t->req->host, "]", NULL);
        }
    }


    if (querystr.len == 0) {
        t->curl_ctx->url = apr_psprintf(t->pool, "%s%s%s",
                                        t->req->proto,
                                        host,
                                        uristr);
    } else {
        t->curl_ctx->url = apr_psprintf(t->pool, "%s%s%s%.*s",
                                        t->req->proto,
                                        host,
                                        uristr,
                                        hdns_to_int(querystr.len),
                                        querystr.data);
    }
    hdns_log_debug("url:%s.", t->curl_ctx->url);

    return HDNS_OK;
}


hdns_http_transport_t *hdns_http_transport_create(hdns_pool_t *p) {
    if (NULL == p) {
        hdns_pool_create(&p, NULL);
    }
    hdns_func_u func;
    hdns_http_transport_t *t;

    t = (hdns_http_transport_t *) hdns_pcalloc(p, sizeof(hdns_http_transport_t));
    t->pool = p;

    t->curl_ctx = (hdns_curl_context_t *) hdns_palloc(p, sizeof(hdns_curl_context_t));
    t->curl_ctx->session = hdns_session_require();
    t->curl_ctx->curl_code = CURLE_OK;
    t->curl_ctx->url = NULL;
    t->curl_ctx->sni = NULL;
    t->curl_ctx->headers = NULL;
    t->curl_ctx->header_callback = hdns_curl_default_header_callback;
    t->curl_ctx->read_callback = hdns_curl_default_read_callback;
    t->curl_ctx->write_callback = hdns_curl_default_write_callback;
    t->curl_ctx->ssl_callback = NULL;

    t->cleanup = hdns_fstack_create(p, 5);
    func.func1 = (hdns_func1_pt) hdns_session_release;
    hdns_fstack_push(t->cleanup, t->curl_ctx->session, func, 1);

    t->req = NULL;
    t->resp = NULL;
    t->controller = NULL;

    return t;
}

static void hdns_move_transport_state(hdns_http_transport_t *t, hdns_transport_state_e s) {
    if (t->resp->extra_info->state < s) {
        t->resp->extra_info->state = s;
    }
}

void hdns_curl_response_headers_parse(hdns_pool_t *p, hdns_table_t *headers, char *buffer, size_t len) {
    char *pos;
    hdns_string_t str;
    hdns_string_t key;
    hdns_string_t value;

    str.data = buffer;
    str.len = len;

    hdns_trip_space_and_cntrl(&str);

    pos = hdns_strlchr(str.data, str.data + str.len, ':');
    if (pos == NULL) {
        return;
    }
    key.data = str.data;
    key.len = pos - str.data;

    pos += 1;
    value.len = str.data + str.len - pos;
    value.data = pos;
    hdns_strip_space(&value);

    apr_table_addn(headers, hdns_pstrdup(p, &key), hdns_pstrdup(p, &value));
}

size_t hdns_curl_default_header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    size_t len;
    hdns_http_transport_t *t;

    t = (hdns_http_transport_t *) (userdata);
    len = size * nitems;

    if (t->resp->extra_info->first_byte_time == 0) {
        t->resp->extra_info->first_byte_time = apr_time_now();
    }

    hdns_curl_response_headers_parse(t->pool, t->resp->headers, buffer, len);

    hdns_move_transport_state(t, TRANS_STATE_HEADER);

    return len;
}

static void hdns_curl_transport_headers_done(hdns_http_transport_t *t) {
    long http_code;
    CURLcode code;

    if (t->resp->extra_info->error_code != HDNS_OK) {
        hdns_log_debug("has error %d.", t->resp->extra_info->error_code);
        return;
    }

    if (t->resp->status > 0) {
        hdns_log_trace("http response status %d.", t->resp->status);
        return;
    }

    t->resp->status = 0;
    if ((code = curl_easy_getinfo(t->curl_ctx->session, CURLINFO_RESPONSE_CODE, &http_code)) != CURLE_OK) {
        t->resp->extra_info->reason = apr_pstrdup(t->pool, curl_easy_strerror(code));
        t->resp->extra_info->error_code = HDNS_INTERNAL_ERROR;
        hdns_log_error("get response status fail, curl code:%d, reason:%s", code, t->resp->extra_info->reason);
        return;
    } else {
        t->resp->status = hdns_to_int(http_code);
    }
}

size_t hdns_curl_default_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t len;
    size_t bytes;
    hdns_http_transport_t *t;

    t = (hdns_http_transport_t *) (userdata);
    len = size * nmemb;

    if (t->resp->extra_info->first_byte_time == 0) {
        t->resp->extra_info->first_byte_time = apr_time_now();
    }

    hdns_curl_transport_headers_done(t);

    if (t->resp->extra_info->error_code != HDNS_OK) {
        hdns_log_debug("write callback abort");
        return 0;
    }

    if ((bytes = t->resp->write_body(t->resp, ptr, len)) < 0) {
        hdns_log_debug("write body failure, %zu.", bytes);
        t->resp->extra_info->error_code = HDNS_WRITE_BODY_ERROR;
        t->resp->extra_info->reason = "write body failure.";
        return 0;
    }

    hdns_move_transport_state(t, TRANS_STATE_BODY_IN);

    return bytes;
}

size_t hdns_curl_default_read_callback(char *buffer, size_t size, size_t nitems, void *instream) {
    size_t len;
    size_t bytes;
    hdns_http_transport_t *t;

    t = (hdns_http_transport_t *) (instream);
    len = size * nitems;

    if (t->resp->extra_info->error_code != HDNS_OK) {
        hdns_log_debug("abort read callback.");
        return CURL_READFUNC_ABORT;
    }

    if ((bytes = t->req->read_body(t->req, buffer, len)) < 0) {
        hdns_log_debug("read body failure, %zu.", bytes);
        t->resp->extra_info->error_code = HDNS_READ_BODY_ERROR;
        t->resp->extra_info->reason = "read body failure.";
        return CURL_READFUNC_ABORT;
    }

    hdns_move_transport_state(t, TRANS_STATE_BODY_OUT);

    return bytes;
}

static int hdns_curl_code_to_status(CURLcode code) {
    switch (code) {
        case CURLE_OUT_OF_MEMORY:
            return HDNS_OUT_MEMORY;
        case CURLE_COULDNT_RESOLVE_PROXY:
        case CURLE_COULDNT_RESOLVE_HOST:
            return HDNS_NAME_LOOKUP_ERROR;
        case CURLE_COULDNT_CONNECT:
            return HDNS_FAILED_CONNECT;
        case CURLE_WRITE_ERROR:
        case CURLE_OPERATION_TIMEDOUT:
            return HDNS_CONNECTION_FAILED;
        case CURLE_PARTIAL_FILE:
            return HDNS_OK;
        case CURLE_SSL_CACERT:
            return HDNS_FAILED_VERIFICATION;
        default:
            return HDNS_INTERNAL_ERROR;
    }
}

static void hdns_curl_transport_finish(hdns_http_transport_t *t) {
    CURLcode code;
    double total_time;
    if ((code = curl_easy_getinfo(t->curl_ctx->session, CURLINFO_TOTAL_TIME, &total_time)) != CURLE_OK) {
        t->resp->extra_info->reason = apr_pstrdup(t->pool, curl_easy_strerror(code));
        t->resp->extra_info->error_code = HDNS_INTERNAL_ERROR;
        hdns_log_error("get total time fail, curl code:%d, reason:%s", code, t->resp->extra_info->reason);
        return;
    } else {
        t->resp->extra_info->total_time = (int32_t) (total_time * APR_USEC_PER_SEC);
    }

    double connect_time;
    if ((code = curl_easy_getinfo(t->curl_ctx->session, CURLINFO_CONNECT_TIME, &connect_time)) != CURLE_OK) {
        t->resp->extra_info->reason = apr_pstrdup(t->pool, curl_easy_strerror(code));
        t->resp->extra_info->error_code = HDNS_INTERNAL_ERROR;
        hdns_log_error("get connect time fail, curl code:%d, reason:%s", code, t->resp->extra_info->reason);
        return;
    } else {
        t->resp->extra_info->connect_time = (int32_t) (connect_time * APR_USEC_PER_SEC);
    }
    if (t->cleanup != NULL) {
        hdns_fstack_destory(t->cleanup);
        t->cleanup = NULL;
    }
}

int hdns_curl_transport_setup(hdns_http_transport_t *t) {

#define curl_easy_setopt_safe(opt, val)  curl_easy_setopt(t->curl_ctx->session, opt, val)

    // curl_ctx
    curl_easy_setopt_safe(CURLOPT_PRIVATE, t);

    if (t->curl_ctx->header_callback != NULL) {
        curl_easy_setopt_safe(CURLOPT_HEADERDATA, t);
        curl_easy_setopt_safe(CURLOPT_HEADERFUNCTION, t->curl_ctx->header_callback);
    }
    if (t->curl_ctx->read_callback != NULL) {
        curl_easy_setopt_safe(CURLOPT_READDATA, t);
        curl_easy_setopt_safe(CURLOPT_READFUNCTION, t->curl_ctx->read_callback);
    }
    if (t->curl_ctx->write_callback != NULL) {
        curl_easy_setopt_safe(CURLOPT_WRITEDATA, t);
        curl_easy_setopt_safe(CURLOPT_WRITEFUNCTION, t->curl_ctx->write_callback);
    }

    hdns_init_curl_headers(t);
    curl_easy_setopt_safe(CURLOPT_HTTPHEADER, t->curl_ctx->headers);

    if (hdns_str_start_with(t->req->proto, HDNS_HTTPS_PREFIX)) {
        if (t->curl_ctx->ssl_callback != NULL) {
            curl_easy_setopt_safe(CURLOPT_SSL_CTX_DATA, t);
            curl_easy_setopt_safe(CURLOPT_SSL_CTX_FUNCTION, t->curl_ctx->ssl_callback);
        }

        // controller
        curl_easy_setopt_safe(CURLOPT_SSL_VERIFYPEER, hdns_to_long(t->controller->verify_peer));
        curl_easy_setopt_safe(CURLOPT_SSL_VERIFYHOST, hdns_to_long(t->controller->verify_host));
        if (t->controller->using_http2) {
#ifdef CURL_HTTP_VERSION_2
            curl_easy_setopt_safe(CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
#endif
        }

#if defined(_WIN32)
        curl_easy_setopt_safe(CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif
        if (t->controller->ca_path != NULL) {
            curl_easy_setopt_safe(CURLOPT_CAPATH, t->controller->ca_path);
        }

        if (t->controller->ca_file != NULL) {
            curl_easy_setopt_safe(CURLOPT_CAINFO, t->controller->ca_file);
        }
        hdns_init_curl_sni(t);
        if (t->curl_ctx->sni != NULL) {
            curl_easy_setopt_safe(CURLOPT_RESOLVE, t->curl_ctx->sni);
        }
    }
    curl_easy_setopt_safe(CURLOPT_TIMEOUT_MS, t->controller->timeout);
    curl_easy_setopt_safe(CURLOPT_CONNECTTIMEOUT_MS, t->controller->connect_timeout);

    if (t->controller->proxy_host != NULL) {
        // proxy
        curl_easy_setopt_safe(CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
        curl_easy_setopt_safe(CURLOPT_PROXY, t->controller->proxy_host);
        // authorize
        if (t->controller->proxy_auth != NULL) {
            curl_easy_setopt_safe(CURLOPT_PROXYAUTH, CURLAUTH_BASIC);
            curl_easy_setopt_safe(CURLOPT_PROXYUSERPWD, t->controller->proxy_auth);
        }
    }

    //req
    if (hdns_init_curl_url(t) != HDNS_OK) {
        return t->resp->extra_info->error_code;
    }
    curl_easy_setopt_safe(CURLOPT_URL, t->curl_ctx->url);
    curl_easy_setopt_safe(CURLOPT_USERAGENT, t->req->user_agent);

    switch (t->req->method) {
        case HDNS_HTTP_HEAD:
            curl_easy_setopt_safe(CURLOPT_NOBODY, 1);
            break;
        case HDNS_HTTP_PUT:
            curl_easy_setopt_safe(CURLOPT_UPLOAD, 1);
            break;
        case HDNS_HTTP_POST:
            curl_easy_setopt_safe(CURLOPT_POST, 1);
            break;
        case HDNS_HTTP_DELETE:
            curl_easy_setopt_safe(CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
        default: // HDNS_HTTP_GET
            break;
    }

    if (hdns_log_level >= HDNS_LOG_DEBUG) {
        curl_easy_setopt_safe(CURLOPT_VERBOSE, 1L);
        curl_easy_setopt_safe(CURLOPT_DEBUGFUNCTION, hdns_curl_debug_callback);
    }
    // other
    curl_easy_setopt_safe(CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt_safe(CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt_safe(CURLOPT_TCP_NODELAY, 1);
    curl_easy_setopt_safe(CURLOPT_NETRC, CURL_NETRC_IGNORED);

#undef curl_easy_setopt_safe

    t->resp->extra_info->state = TRANS_STATE_INIT;

    return HDNS_OK;
}

int hdns_http_transport_perform(hdns_http_transport_t *t) {
    int ecode;
    CURLcode code;
    ecode = hdns_curl_transport_setup(t);
    if (ecode != HDNS_OK) {
        return ecode;
    }
    t->resp->extra_info->start_time = apr_time_now();
    code = curl_easy_perform(t->curl_ctx->session);
    t->resp->extra_info->finish_time = apr_time_now();
    hdns_move_transport_state(t, TRANS_STATE_DONE);
    t->curl_ctx->curl_code = code;
    hdns_curl_transport_finish(t);
    if ((code != CURLE_OK) && (t->resp->extra_info->error_code == HDNS_OK)) {
        ecode = hdns_curl_code_to_status(code);
        if (ecode != HDNS_OK) {
            t->resp->extra_info->error_code = ecode;
            t->resp->extra_info->reason = apr_pstrdup(t->pool, curl_easy_strerror(code));
            hdns_log_error("transport failure curl code:%d error:%s", code, t->resp->extra_info->reason);
        }
    }
    return t->resp->extra_info->error_code;
}
