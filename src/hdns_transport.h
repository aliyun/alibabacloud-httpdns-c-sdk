//
// Created by cagaoshuai on 2024/3/22.
//

#ifndef HDNS_C_SDK_HDNS_TRANSPORT_H
#define HDNS_C_SDK_HDNS_TRANSPORT_H

#include "hdns_define.h"
#include "hdns_list.h"


HDNS_CPP_START

typedef struct hdns_http_request_s hdns_http_request_t;
typedef struct hdns_http_response_s hdns_http_response_t;

typedef size_t (*hdns_http_write_body_fn_t)(hdns_http_response_t *resp, const char *buffer, size_t size);

typedef size_t (*hdns_http_read_body_fn_t)(hdns_http_request_t *req, char *buffer, size_t size);


typedef struct {
    hdns_pool_t *pool;
    int32_t timeout;
    int32_t connect_timeout;
    char *proxy_host;
    char *proxy_auth;
    bool verify_peer;
    bool verify_host;
    char *ca_path;
    char *ca_file;
    char *ca_host;
    bool using_http2;
} hdns_http_controller_t;

typedef enum {
    TRANS_STATE_INIT,
    TRANS_STATE_HEADER,
    TRANS_STATE_BODY_IN,
    TRANS_STATE_BODY_OUT,
    TRANS_STATE_DONE
} hdns_transport_state_e;

typedef struct {
    hdns_pool_t *pool;
    int64_t start_time;
    int64_t first_byte_time;
    int64_t finish_time;
    int32_t connect_time;
    int32_t total_time;
    hdns_transport_state_e state;
    int32_t error_code;
    char *reason;
} hdns_http_info_t;

struct hdns_http_response_s {
    hdns_pool_t *pool;
    int32_t status;
    hdns_table_t *headers;
    hdns_list_head_t *body;
    int64_t body_len;
    hdns_http_write_body_fn_t write_body;
    hdns_http_info_t *extra_info;
};

struct hdns_http_request_s {
    hdns_pool_t *pool;
    http_method_e method;
    char *proto;
    char *host;
    char *uri;
    hdns_table_t *headers;
    hdns_table_t *query_params;
    hdns_list_head_t *body;
    int64_t body_len;
    char *user_agent;
    hdns_http_read_body_fn_t read_body;
};

typedef struct {
    // curl information
    CURL *session;
    CURLcode curl_code;
    char *url;
    struct curl_slist *sni;
    struct curl_slist *headers;
    curl_read_callback header_callback;
    curl_read_callback read_callback;
    curl_write_callback write_callback;
    curl_ssl_ctx_callback ssl_callback;
} hdns_curl_context_t;

typedef struct {
    hdns_pool_t *pool;
    hdns_http_request_t *req;
    hdns_http_response_t *resp;
    hdns_http_controller_t *controller;
    hdns_array_header_t *cleanup;
    hdns_curl_context_t *curl_ctx;
} hdns_http_transport_t;

hdns_http_transport_t *hdns_http_transport_create(hdns_pool_t *p);

int hdns_http_transport_perform(hdns_http_transport_t *t);

HDNS_CPP_END


#endif
