//
// Created by cagaoshuai on 2024/1/15.
//

#include "httpdns_http.h"
#include "httpdns_global_config.h"

static int32_t test_exchange_singel_request(char *url) {
    httpdns_http_context_t *http_context = httpdns_http_context_create(url, 10000);
    int32_t ret = httpdns_http_single_exchange(http_context);
    httpdns_http_context_print(http_context);
    httpdns_http_context_destroy(http_context);
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
    NEW_EMPTY_LIST_IN_STACK(http_contexts);
    httpdns_http_context_t *http_context = httpdns_http_context_create(
            "https://203.107.1.34/139450/d?host=www.baidu.com", 10000);
    httpdns_list_add(&http_contexts, http_context, NULL);

    http_context = httpdns_http_context_create(
            "https://203.107.1.34/139450/resolve?host=www.aliyun.com,qq.com,www.taobao.com,help.aliyun.com", 10000);
    httpdns_list_add(&http_contexts, http_context, NULL);

    http_context = httpdns_http_context_create(
            "https://203.107.1.34/139450/d?host=www.163.com", 10000);
    httpdns_list_add(&http_contexts, http_context, NULL);

    http_context = httpdns_http_context_create(
            "https://203.107.1.34/139450/d?host=huaweicloud.com", 10000);
    httpdns_list_add(&http_contexts, http_context, NULL);

    int32_t ret = httpdns_http_multiple_exchange(&http_contexts);
    if (ret == HTTPDNS_SUCCESS) {
        printf("\n");
        httpdns_list_print(&http_contexts, DATA_PRINT_FUNC(httpdns_http_context_print));
    }
    httpdns_list_free(&http_contexts, DATA_FREE_FUNC(httpdns_http_context_destroy));
    return ret;
}


int main(void) {
    init_httpdns_sdk();
    int32_t success = HTTPDNS_SUCCESS;
    success |= test_exchange_singel_request_with_resolve();
    success |= test_exchange_singel_request_with_schedule();
    success |= test_exchange_multi_request_with_resolve();
    cleanup_httpdns_sdk();
    return success;
}