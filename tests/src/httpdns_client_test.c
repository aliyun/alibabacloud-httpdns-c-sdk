////
//// Created by cagaoshuai on 2024/1/20.
////
//#include "httpdns_client.h"
//#include "httpdns_http.h"
//
//static httpdns_config_t *get_test_config() {
//    httpdns_config_t *config = create_httpdns_config();
//    httpdns_config_set_account_id(config, "100000");
//    httpdns_config_set_using_https(config, true);
//    httpdns_config_set_secret_key(config, "29b79c1d12d6a30055f138a37f3f210f");
//    httpdns_config_set_using_sign(config, true);
//    httpdns_config_set_using_https(config, true);
//    return config;
//}
//
//static int32_t test_httpdns_client_simple_resolve() {
//    httpdns_config_t *config = get_test_config();
//    httpdns_client_t *client = httpdns_client_create(config);
//    httpdns_resolve_result_t * result;
//    httpdns_client_simple_resolve(client, "www.aliyun.com", HTTPDNS_QUERY_TYPE_BOTH, NULL,  &result);
//    printf("\n");
//    httpdns_resolve_result_print(result);
//    httpdns_resolve_result_destroy(result);
//    sleep(5);
//    httpdns_client_simple_resolve(client, "www.aliyun.com", HTTPDNS_QUERY_TYPE_BOTH, NULL,  &result);
//    printf("\n");
//    httpdns_resolve_result_print(result);
//    int32_t ret = (NULL != result && result->hit_cache) ? HTTPDNS_SUCCESS: HTTPDNS_FAILURE;
//    httpdns_resolve_result_destroy(result);
//    destroy_httpdns_config(config);
//    httpdns_client_destroy(client);
//    return ret;
//}
//
//
////int main(void) {
////    int32_t success = HTTPDNS_SUCCESS;
////    success |= test_httpdns_client_simple_resolve();
////    return success;
////}