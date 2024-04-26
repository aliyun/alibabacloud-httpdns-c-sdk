//
// Created by cagaoshuai on 2024/3/26.
//


#include "test_suit_list.h"
#include "hdns_buf.h"


void hdns_test_transport_get(CuTest *tc) {
    hdns_sdk_init();
    hdns_log_level = HDNS_LOG_DEBUG;
    hdns_pool_new(pool);
    hdns_http_transport_t *transport = hdns_http_transport_create(pool);
    hdns_http_controller_t *ctl = hdns_http_controller_create(pool);
    hdns_http_request_t *req = hdns_http_request_create(pool);
    hdns_http_response_t *resp = hdns_http_response_create(pool);

    transport->req = req;
    transport->resp = resp;
    transport->controller = (hdns_http_controller_t *) ctl;

    req->proto = HDNS_HTTPS_PREFIX;
    req->host = "203.107.1.1";
    req->uri = "/100000/d";
    apr_table_set(req->query_params, "host", "www.aliyun.com");
    apr_table_set(req->query_params, "query", "4,6");
    apr_table_set(req->query_params, "ip", "1.1.1.1");
    apr_table_set(req->headers, "Accept-Encoding", "gzip, deflate, br, zstd");
    apr_table_set(req->headers, "Accept-Language", "zh-CN");

    int err_code = hdns_http_transport_perform(transport);

    hdns_log_debug("hdns_test_transport_get response: %s", hdns_buf_list_content(resp->pool, resp->body));

    hdns_pool_destroy(pool);
    hdns_sdk_cleanup();
    CuAssert(tc, "Transport HTTP GET请求测试失败", !err_code);
}


void add_hdns_transport_tests(CuSuite * suite) {
    SUITE_ADD_TEST(suite, hdns_test_transport_get);
}