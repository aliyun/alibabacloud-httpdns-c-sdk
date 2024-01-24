//
// Created by cagaoshuai on 2024/1/10.
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
    data_print_function_ptr_t private_data_print_func;
    data_clone_function_ptr_t private_data_clone_func;
    data_cmp_function_ptr_t private_data_cmp_func;
    data_free_function_ptr_t private_data_free_func;
} httpdns_http_context_t;


httpdns_http_context_t *httpdns_http_context_create(char *url, int32_t timeout_ms);

int32_t httpdns_http_context_set_private_data(httpdns_http_context_t *http_context,
                                              void *private_data,
                                              data_print_function_ptr_t private_data_print_func,
                                              data_clone_function_ptr_t private_data_clone_func,
                                              data_cmp_function_ptr_t private_data_cmp_func,
                                              data_free_function_ptr_t private_data_free_func
);

int32_t httpdns_http_context_set_user_agent(httpdns_http_context_t *http_context, const char *user_agent);

void httpdns_http_context_print(httpdns_http_context_t *http_context);

httpdns_http_context_t *httpdns_http_context_clone(httpdns_http_context_t *http_context);

void httpdns_http_context_destroy(httpdns_http_context_t *http_context);

int32_t httpdns_http_single_exchange(httpdns_http_context_t *http_context);

int32_t httpdns_http_multiple_exchange(struct list_head *http_contexts);

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_HTTP_H
