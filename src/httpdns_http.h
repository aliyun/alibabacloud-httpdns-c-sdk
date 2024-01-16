//
// Created by cagaoshuai on 2024/1/10.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_HTTP_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_HTTP_H


#define SSL_VERIFY_HOST    "203.107.1.1"
#define MULTI_HANDLE_TIMEOUT_MS  10
#define HTTP_SCHEME        "http://"
#define HTTPS_SCHEME        "https://"
#define CERT_PEM_NAME       "Cert:"
#define HTTP_STATUS_OK 200
#define IS_HTTPS_SCHEME(URL) \
(strncmp(URL, HTTPS_SCHEME, strlen(HTTPS_SCHEME)) == 0)


#include<stdint.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <string.h>
#include "httpdns_list.h"
#include "sds.h"

typedef struct {
    char *url;
    int32_t http_status;
    char *body;
    int32_t total_time_ms;
    char *cache_key;
} httpdns_http_response_t;

typedef struct {
    char *url;
    int32_t timeout_ms;
    char *cache_key;
} httpdns_http_request_t;

httpdns_http_request_t *clone_httpdns_http_request(const httpdns_http_request_t *request);

httpdns_http_request_t *create_httpdns_http_request(char *url, int32_t timeout_ms, char *cache_key);

void destroy_httpdns_http_request(httpdns_http_request_t *request);

void destroy_httpdns_http_requests(struct list_head *requests);

httpdns_http_response_t *create_httpdns_http_response(char *url, char *cache_key);

httpdns_http_response_t *clone_httpdns_http_response(const httpdns_http_response_t *origin_response);

void
httpdns_http_fill_response(httpdns_http_response_t *response, int32_t http_status, char *body, char *error_message,
                           int64_t time_cost_ms,
                           char *cache_key);

void destroy_httpdns_http_response(httpdns_http_response_t *response);

void destroy_httpdns_http_responses(struct list_head *responses);

int32_t httpdns_http_single_request_exchange(httpdns_http_request_t *request, httpdns_http_response_t **response);

int32_t httpdns_http_multiple_request_exchange(struct list_head *requests, struct list_head *responses);

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_HTTP_H
