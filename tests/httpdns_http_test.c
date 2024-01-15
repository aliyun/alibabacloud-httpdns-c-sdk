//
// Created by cagaoshuai on 2024/1/15.
//

#include "httpdns_http.h"

static int32_t test_exchange_singel_request() {
    char *url = "https://203.107.1.65/139450/d?host=www.baidu.com";
    httpdns_http_request_t *request = create_httpdns_http_request(url, 10000, NULL);
    httpdns_http_response_t *response = httpdns_http_single_request_exchange(request);
    int32_t ret = HTTPDNS_FAILURE;
    if (NULL != response && response->http_status == HTTP_STATUS_OK) {
        ret = HTTPDNS_SUCCESS;
    }
    destroy_httpdns_http_response(response);
    destroy_httpdns_http_request(request);
    return ret;
}


int main(void) {
    if (test_exchange_singel_request() != HTTPDNS_SUCCESS) {
        return -1;
    }
    return 0;
}