//
// Created by caogaoshuai on 2024/2/7.
//
#include "httpdns/httpdns_client_wrapper.h"
#include "httpdns/httpdns_log.h"
#include "httpdns/httpdns_time.h"
#include "httpdns/httpdns_localdns.h"

#define MOCK_BUSINESS_HOST   "www.aliyun.com"
#define MOCK_HTTPDNS_ACCOUNT   "139450"
#define MOCK_BATCH_REQUEST_SIZE    100

int main(int argc, char *argv[]) {
    // 1. HTTPDNS SDK 环境初始化
    httpdns_client_env_init(MOCK_HTTPDNS_ACCOUNT, NULL);
    // 其他自定义配置
    httpdns_config_t *httpdns_config = httpdns_client_get_config();
//    httpdns_config_set_using_https(httpdns_config, true);
//    httpdns_config_set_using_sign(httpdns_config, true);
    httpdns_config_set_using_cache(httpdns_config, false);

    httpdns_log_start();
    // 2. HTTPDNS SDK 解析结果
    NEW_EMPTY_LIST_IN_STACK(hosts);
    for (int i = 0; i < MOCK_BATCH_REQUEST_SIZE; i++) {
        httpdns_list_add(&hosts, MOCK_BUSINESS_HOST, STRING_CLONE_FUNC);
    }
    NEW_EMPTY_LIST_IN_STACK(results);
    struct timeval start_time = httpdns_time_now();
    get_httpdns_results_for_hosts_sync_with_cache(&hosts, HTTPDNS_QUERY_TYPE_AUTO, NULL, &results);
    struct timeval end_time = httpdns_time_now();
    // 请求100次，合并后20个请求，20个结果
    log_trace("sync server in linux example, httpdns cost %ld ms/个, request number %d, result number %d",
              httpdns_time_diff(end_time, start_time) / MOCK_BATCH_REQUEST_SIZE,
              MOCK_BATCH_REQUEST_SIZE,
              httpdns_list_size(&results));
    // 3. 轮询获取结果
    httpdns_list_for_each_entry(result_cursor, &results) {
        sds result_str = httpdns_resolve_result_to_string(result_cursor->data);
        printf("%s\n", result_str);
        sdsfree(result_str);
    }
    // 4. 解析结果释放
    httpdns_list_free(&results, DATA_FREE_FUNC(httpdns_resolve_result_free));
    httpdns_list_free(&hosts, STRING_FREE_FUNC);
    // 5. HTTPDNS SDK 环境释放
    httpdns_client_env_cleanup();
    httpdns_log_stop();
    return 0;
}
