//
// Created by cagaoshuai on 2024/1/17.
//
#include "httpdns_scheduler.h"
#include "httpdns_global_config.h"

static int32_t test_refresh_resolve_servers() {
    httpdns_config_t *config = create_httpdns_config();
    httpdns_config_set_account_id(config, "100000");
    httpdns_config_set_using_https(config, true);
    httpdns_scheduler_t *scheduler = create_httpdns_scheduler(config);
    int32_t ret = httpdns_scheduler_refresh_resolve_servers(scheduler);
    httpdns_scheduler_print_resolve_servers(scheduler);
    destroy_httpdns_config(config);
    destroy_httpdns_scheduler(scheduler);
    return ret ? HTTPDNS_FAILURE : HTTPDNS_SUCCESS;
}

static int32_t test_httpdns_scheduler_get_resolve_server() {
    httpdns_config_t *config = create_httpdns_config();
    httpdns_config_set_account_id(config, "100000");
    httpdns_config_set_using_https(config, true);
    httpdns_scheduler_t *scheduler = create_httpdns_scheduler(config);
    int32_t ret = httpdns_scheduler_refresh_resolve_servers(scheduler);
    if (!ret) {
        for (int i = 0; i < 100; i++) {
            char *server_name;
            httpdns_scheduler_get_resolve_server(scheduler, &server_name);
            if (NULL != server_name) {
                 char* url = sdsnew(HTTPS_SCHEME);
                 url = sdscat(url, server_name);
                 url = sdscat(url, "/100000/d?host=www.taobao.com");
                // 构建请求
                httpdns_http_request_t *request = create_httpdns_http_request(url, 10000, NULL);
                httpdns_http_response_t *response;
                ret = httpdns_http_single_request_exchange(request, &response);
                if (!ret) {
                    httpdns_http_print_response(response);
                    httpdns_scheduler_update_server_rt(scheduler, server_name, response->total_time_ms);
                    httpdns_scheduler_print_resolve_servers(scheduler);
                    destroy_httpdns_http_response(response);
                }
                destroy_httpdns_http_request(request);
                sdsfree(url);
                sdsfree(server_name);
            }
        }
    }
    destroy_httpdns_config(config);
    destroy_httpdns_scheduler(scheduler);
    return ret ? HTTPDNS_FAILURE : HTTPDNS_SUCCESS;
}


int main(void) {
    init_httpdns_sdk();
    test_refresh_resolve_servers();
    test_httpdns_scheduler_get_resolve_server();
    cleanup_httpdns_sdk();
    return 0;
}