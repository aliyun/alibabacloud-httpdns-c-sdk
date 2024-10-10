#include "hdns_list.h"
#include "hdns_buf.h"
#include "hdns_http.h"
#include "hdns_define.h"
#include "apr_thread_mutex.h"
#include <apr_file_io.h>

static size_t hdns_http_read_body(hdns_http_request_t *req, char *buffer, size_t len);

static size_t hdns_http_write_body(hdns_http_response_t *resp, const char *buffer, size_t len);

size_t hdns_http_read_body(hdns_http_request_t *req, char *buffer, size_t len) {
    int wsize;
    int bytes = 0;
    hdns_buf_t *b;

    hdns_list_for_each_entry_safe(buf_cursor, req->body) {
        b = buf_cursor->data;
        wsize = hdns_buf_size(b);
        if (wsize == 0) {
            hdns_list_del(buf_cursor);
            continue;
        }
        wsize = hdns_min(len - bytes, wsize);
        if (wsize == 0) {
            break;
        }
        memcpy(buffer + bytes, b->pos, wsize);
        b->pos += wsize;
        bytes += wsize;
        if (b->pos == b->last) {
            hdns_list_del(buf_cursor);
        }
    }

    return bytes;
}

size_t hdns_http_write_body(hdns_http_response_t *resp, const char *buffer, size_t len) {
    hdns_buf_t *b;
    b = hdns_create_buf(resp->body->pool, len);
    memcpy(b->pos, buffer, len);
    b->last += len;
    hdns_list_add(resp->body, b, NULL);
    resp->body_len += len;

    return len;
}

hdns_http_controller_t *hdns_http_controller_create(hdns_pool_t *p) {
    if (NULL == p) {
        hdns_pool_create(&p, NULL);
    }
    hdns_http_controller_t *ctl;
    ctl = (hdns_http_controller_t *) hdns_pcalloc(p, sizeof(hdns_http_controller_t));
    ctl->pool = p;
    ctl->timeout = HDNS_MAX_TIMEOUT_MS;
    ctl->connect_timeout = HDNS_MAX_CONNECT_TIMEOUT_MS;
    ctl->proxy_host = NULL;
    ctl->proxy_auth = NULL;
    ctl->verify_peer = TRUE;
    ctl->verify_host = TRUE;
    ctl->ca_path = NULL;
    ctl->ca_file = NULL;
    ctl->ca_host = HDNS_SSL_CA_HOST;
    ctl->using_http2 = FALSE;

    return ctl;
}

hdns_http_request_t *hdns_http_request_create(hdns_pool_t *p) {
    if (NULL == p) {
        hdns_pool_create(&p, NULL);
    }
    hdns_http_request_t *req;
    req = (hdns_http_request_t *) hdns_pcalloc(p, sizeof(hdns_http_request_t));
    req->pool = p;
    req->method = HDNS_HTTP_GET;
    req->proto = HDNS_HTTPS_PREFIX;
    req->host = NULL;
    req->uri = NULL;
    req->headers = hdns_table_make(p, 5);
    req->query_params = hdns_table_make(p, 5);
    req->body = hdns_list_new(p);
    req->body_len = 0;
    req->user_agent = HDNS_VER;
    req->read_body = hdns_http_read_body;

    return req;
}

hdns_http_response_t *hdns_http_response_create(hdns_pool_t *p) {
    if (NULL == p) {
        hdns_pool_create(&p, NULL);
    }
    hdns_http_response_t *resp;
    hdns_http_info_t *extra_info;

    extra_info = (hdns_http_info_t *) hdns_palloc(p, sizeof(hdns_http_info_t));
    extra_info->pool = p;
    extra_info->start_time = 0;
    extra_info->first_byte_time = 0;
    extra_info->finish_time = 0;
    extra_info->connect_time = 0;
    extra_info->total_time = 0;
    extra_info->error_code = HDNS_OK;
    extra_info->reason = NULL;

    resp = (hdns_http_response_t *) hdns_pcalloc(p, sizeof(hdns_http_response_t));
    resp->pool = p;
    resp->status = -1;
    resp->headers = hdns_table_make(p, 3);
    resp->body = hdns_list_new(p);
    resp->body_len = 0;
    resp->write_body = hdns_http_write_body;
    resp->extra_info = extra_info;
    return resp;
}

int hdns_http_send_request(hdns_http_controller_t *ctl, hdns_http_request_t *req, hdns_http_response_t *resp) {
    hdns_http_transport_t *t;
    t = hdns_http_transport_create(ctl->pool);
    t->req = req;
    t->resp = resp;
    t->controller = (hdns_http_controller_t *) ctl;
    return hdns_http_transport_perform(t);
}

bool hdns_http_should_retry(hdns_http_response_t *http_resp) {
    // HTTP建链失败或500状态码，进行重试
    return (5 == http_resp->status / 100);
}
