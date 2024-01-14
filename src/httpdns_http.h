//
// Created by cagaoshuai on 2024/1/10.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_HTTP_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_HTTP_H


#define SSL_VERIFY_HOST    "203.107.1.1"
#define MULTI_HANDLE_TIMEOUT_MS  10
#define HTTP_SCHEME        "http://"
#define HTTPS_SCHEME        "https://"
#define HTTP_STATUS_OK 200

#include<stdint.h>
#include <stdbool.h>
#include <curl/curl.h>
#include "httpdns_list.h"
#include "sds.h"

typedef struct _httpdns_http_response {
    char *url;
    int32_t http_status;
    char *body;
    int64_t total_time_ms;
    char * cache_key;
} httpdns_http_response_t;

typedef struct _httpdns_http_request {
    char *url;
    int64_t timeout_ms;
    char * cache_key;
} httpdns_http_request_t;

httpdns_http_request_t *create_httpdns_http_request(char *url, int64_t timeout_ms, char *cache_key);

void destroy_httpdns_http_request(httpdns_http_request_t *request);

void destroy_httpdns_http_requests(struct list_head *requests);

httpdns_http_response_t * create_httpdns_http_response(char *url);

void
httpdns_http_fill_response(httpdns_http_response_t *response, int32_t http_status, char *body, char *error_message, int64_t time_cost_ms,
                           char *cache_key);
void destroy_httpdns_http_response(httpdns_http_response_t *response);

void destroy_httpdns_http_responses(struct list_head *responses);

httpdns_http_response_t* httpdns_http_single_request_exchange(httpdns_http_request_t *request);

struct list_head httpdns_http_multiple_request_exchange(struct list_head *requests);

int32_t httpdns_http_init();

void httpdns_http_destroy();

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_HTTP_H
