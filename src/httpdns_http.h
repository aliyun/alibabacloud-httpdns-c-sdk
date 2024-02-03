//
//负责与HTTPDNS服务端的HTTP交互，主要包括请求和响应
//
// Created by caogaoshuai on 2024/1/10.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_HTTP_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_HTTP_H

#include<stdint.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <string.h>
#include "httpdns_list.h"
#include "sds.h"

#define SSL_VERIFY_HOST    "203.107.1.1"
#define MIN_HTTP_REQUEST_TIMEOUT_MS  10
#define MAX_HTTP_REQUEST_TIMEOUT_MS  5000
#define HTTP_SCHEME        "http://"
#define HTTPS_SCHEME        "https://"
#define CERT_PEM_NAME       "Cert:"
#define HTTP_STATUS_OK 200
#define IS_HTTPS_SCHEME(URL) \
    (strncmp(URL, HTTPS_SCHEME, strlen(HTTPS_SCHEME)) == 0)

typedef struct {
    char *request_url;
    int32_t request_timeout_ms;
    char *user_agent;
    char *response_body;
    int32_t response_status;
    int32_t response_rt_ms;
    void *private_data;
} httpdns_http_context_t;

/**
 * must free using httpdns_http_context_free
 */
httpdns_http_context_t *httpdns_http_context_new(const char *url, int32_t timeout_ms);

int32_t httpdns_http_context_set_private_data(httpdns_http_context_t *http_context, void *private_data);

int32_t httpdns_http_context_set_user_agent(httpdns_http_context_t *http_context, const char *user_agent);
/**
 * must free using sdsfree
 */
sds httpdns_http_context_to_string(const httpdns_http_context_t *http_context);

void httpdns_http_context_free(httpdns_http_context_t *http_context);

int32_t httpdns_http_single_exchange(httpdns_http_context_t *http_context);

int32_t httpdns_http_multiple_exchange(struct list_head *http_contexts);

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_HTTP_H
