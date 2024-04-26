//
// Created by cagaoshuai on 2024/3/26.
//


#include "test_suit_list.h"
#include "hdns_buf.h"


void hdns_test_http_get(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_pool_new(pool);
    hdns_http_controller_t *ctl = hdns_http_controller_create(pool);
    hdns_http_request_t *req = hdns_http_request_create(pool);
    hdns_http_response_t *resp = hdns_http_response_create(pool);

    req->proto = HDNS_HTTPS_PREFIX;
    req->host = "47.74.222.190";
    req->uri = "/100000/d";
    apr_table_set(req->query_params, "host", "www.aliyun.com");
    apr_table_set(req->query_params, "query", "4,6");
    apr_table_set(req->query_params, "ip", "1.1.1.1");
    apr_table_set(req->headers, "Accept-Encoding", "gzip, deflate, br, zstd");
    apr_table_set(req->headers, "Accept-Language", "zh-CN");

    int err_code = hdns_http_send_request(ctl, req, resp);

    hdns_log_info("hdns_test_http_get response: %s", hdns_buf_list_content(resp->pool, resp->body));

    hdns_pool_destroy(pool);
    hdns_sdk_cleanup();
    CuAssert(tc, "HTTP GET请求测试失败", !err_code);
}

void hdns_test_http_post(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_pool_new(pool);
    hdns_http_controller_t *ctl = hdns_http_controller_create(pool);
    hdns_http_request_t *req = hdns_http_request_create(pool);
    hdns_http_response_t *resp = hdns_http_response_create(pool);

    req->method = HDNS_HTTP_POST;
    req->proto = HDNS_HTTP_PREFIX;
    req->host = "203.107.1.1";
    req->uri = "/100000/ss";

    apr_table_set(req->headers, "Accept-Encoding", "gzip, deflate, br, zstd");
    apr_table_set(req->headers, "Accept-Language", "zh-CN");
    apr_table_set(req->headers, "Content-Type", "application/x-www-form-urlencoded");


    hdns_buf_t *b = hdns_create_buf(req->body->pool, 32);
    char region[] = "region=sg";
    int region_len = strlen(region);
    hdns_buf_append_string(req->pool, b, region, region_len);
    hdns_list_add(req->body, b, NULL);
    req->body_len += region_len;

    b = hdns_create_buf(req->body->pool, 32);
    char sdk[] = "&sdkVersion=1.0.0";
    int sdk_len = strlen(sdk);
    hdns_buf_append_string(req->pool, b, sdk, sdk_len);
    hdns_list_add(req->body, b, NULL);
    req->body_len += sdk_len;

    b = hdns_create_buf(req->body->pool, 32);
    char platform[] = "&platform=linux";
    int platform_len = strlen(platform);
    hdns_buf_append_string(req->pool, b, platform, platform_len);
    hdns_list_add(req->body, b, NULL);
    req->body_len += platform_len;

    int err_code = hdns_http_send_request(ctl, req, resp);

    hdns_log_info("hdns_test_http_post response: %s", hdns_buf_list_content(resp->pool, resp->body));

    hdns_pool_destroy(pool);
    hdns_sdk_cleanup();
    CuAssert(tc, "HTTP POST请求测试失败", !err_code);
}


void add_hdns_http_tests(CuSuite *suite) {
    SUITE_ADD_TEST(suite, hdns_test_http_get);
    SUITE_ADD_TEST(suite, hdns_test_http_post);
}