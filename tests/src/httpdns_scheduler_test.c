////
//// Created by cagaoshuai on 2024/1/17.
////
//#include "httpdns_scheduler.h"
//#include "httpdns_global_config.h"
//
//static int32_t test_refresh_resolve_servers() {
//    httpdns_config_t *config = create_httpdns_config();
//    httpdns_config_set_account_id(config, "100000");
//    httpdns_config_set_using_https(config, true);
//    httpdns_scheduler_t *scheduler = httpdns_scheduler_create(config);
//    int32_t ret = httpdns_scheduler_refresh(scheduler);
//    printf("\n");
//    httpdns_scheduler_print(scheduler);
//    destroy_httpdns_config(config);
//    httpdns_scheduler_destroy(scheduler);
//    return ret ? HTTPDNS_FAILURE : HTTPDNS_SUCCESS;
//}
//
//static int32_t test_httpdns_scheduler_get_resolve_server() {
//    httpdns_config_t *config = create_httpdns_config();
//    httpdns_config_set_account_id(config, "100000");
//    httpdns_config_set_using_https(config, true);
//    httpdns_config_set_secret_key(config, "29b79c1d12d6a30055f138a37f3f210f");
//    httpdns_config_set_using_sign(config, true);
//    httpdns_scheduler_t *scheduler = httpdns_scheduler_create(config);
//    int32_t ret = httpdns_scheduler_refresh(scheduler);
//    if (!ret) {
//        for (int i = 0; i < 20; i++) {
//            char* server_name = httpdns_scheduler_get(scheduler);
//            if (NULL != server_name) {
//                 char* url = sdsnew(HTTPS_SCHEME);
//                 url = sdscat(url, server_name);
//                 url = sdscat(url, "/100000/d?host=www.taobao.com");
//                // 构建请求
//                httpdns_http_context_t *http_context = httpdns_http_context_create(url, 10000);
//                ret = httpdns_http_single_exchange(http_context);
//                if (!ret) {
//                    printf("\n");
//                    httpdns_http_context_print(http_context);
//                    httpdns_scheduler_update(scheduler, server_name, http_context->response_rt_ms);
//                    printf("\n");
//                    httpdns_scheduler_print(scheduler);
//                }
//                httpdns_http_context_destroy(http_context);
//                sdsfree(url);
//                sdsfree(server_name);
//            }
//        }
//    }
//    destroy_httpdns_config(config);
//    httpdns_scheduler_destroy(scheduler);
//    return ret ? HTTPDNS_FAILURE : HTTPDNS_SUCCESS;
//}
//
//
////int main(void) {
////    init_httpdns_sdk();
////    int32_t success = HTTPDNS_SUCCESS;
////    success |= test_refresh_resolve_servers();
////    success |= test_httpdns_scheduler_get_resolve_server();
////    cleanup_httpdns_sdk();
////    return success;
////}