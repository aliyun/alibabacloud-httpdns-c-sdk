//
// Created by cagaoshuai on 2024/1/15.
//

#include "httpdns_http.h"
#include "httpdns_global_config.h"

static int32_t test_exchange_singel_request(char *url) {
    httpdns_http_request_t *request = create_httpdns_http_request(url, 10000, NULL);
    httpdns_http_response_t *response;
    int32_t ret = httpdns_http_single_request_exchange(request, &response);
    if (!ret) {
        httpdns_http_print_response(response);
        destroy_httpdns_http_response(response);
    }
    destroy_httpdns_http_request(request);
    return ret;
}

static int32_t test_exchange_singel_request_with_resolve() {
    char *url = "https://203.107.1.34/139450/d?host=www.baidu.com";
    return test_exchange_singel_request(url);
}

static int32_t test_exchange_singel_request_with_schedule() {
    char *url = "https://203.107.1.34/139450/ss";
    return test_exchange_singel_request(url);
}


static int32_t test_exchange_multi_request_with_resolve() {
    char *url1 = "https://203.107.1.34/139450/d?host=www.baidu.com";
    httpdns_http_request_t *request1 = create_httpdns_http_request(url1, 10000, NULL);
    char *url2 = "https://203.107.1.34/139450/resolve?host=www.aliyun.com,qq.com,www.taobao.com,help.aliyun.com";
    httpdns_http_request_t *request2 = create_httpdns_http_request(url2, 10000, NULL);
    char *url3 = "https://203.107.1.34/139450/d?host=www.163.com";
    httpdns_http_request_t *request3 = create_httpdns_http_request(url3, 10000, NULL);
    char *url4 = "https://203.107.1.34/139450/d?host=huaweicloud.com";
    httpdns_http_request_t *request4 = create_httpdns_http_request(url4, 10000, NULL);
    struct list_head requests;
    httpdns_list_init(&requests);
    struct list_head responses;
    httpdns_list_init(&responses);

    httpdns_list_add(&requests, request1, DATA_CLONE_FUNC(clone_httpdns_http_request));
    httpdns_list_add(&requests, request2, DATA_CLONE_FUNC(clone_httpdns_http_request));
    httpdns_list_add(&requests, request3, DATA_CLONE_FUNC(clone_httpdns_http_request));
    httpdns_list_add(&requests, request4, DATA_CLONE_FUNC(clone_httpdns_http_request));
    int32_t ret = httpdns_http_multiple_request_exchange(&requests, &responses);
    if (!ret) {
        size_t size = httpdns_list_size(&responses);
        for (int i = 0; i < size; i++) {
            httpdns_http_print_response(httpdns_list_get(&responses, i));
        }
    }
    destroy_httpdns_http_request(request1);
    destroy_httpdns_http_request(request2);
    destroy_httpdns_http_request(request3);
    destroy_httpdns_http_request(request4);
    httpdns_list_free(&requests, DATA_FREE_FUNC(destroy_httpdns_http_request));
    httpdns_list_free(&responses, DATA_FREE_FUNC(destroy_httpdns_http_response));
    return ret;
}



int main(void) {
    init_httpdns_sdk();
    if (test_exchange_singel_request_with_resolve() != HTTPDNS_SUCCESS) {
        cleanup_httpdns_sdk();
        return -1;
    }
    if (test_exchange_singel_request_with_schedule() != HTTPDNS_SUCCESS) {
        cleanup_httpdns_sdk();
        return -1;
    }
    if (test_exchange_multi_request_with_resolve() != HTTPDNS_SUCCESS) {
        cleanup_httpdns_sdk();
        return -1;
    }
    cleanup_httpdns_sdk();
    return 0;
}