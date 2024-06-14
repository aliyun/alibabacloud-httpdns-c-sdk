//
// Created by caogaoshuai on 2024/2/7.
//

#include "hdns_api.h"
#include <curl/curl.h>

#define MOCK_HTTPDNS_ACCOUNT      "139450"
#define MOCK_HTTPDNS_SECRET       NULL


int main(int argc, char *argv[]) {
    hdns_to_void_p(argv);
    hdns_to_int(argc);
    // 1. HTTPDNS SDK 环境初始化
    if (hdns_sdk_init() != HDNS_OK) {
        goto cleanup;
    }
    // 2. 创建客户端（可复用）
    hdns_client_t *client = hdns_client_create(MOCK_HTTPDNS_ACCOUNT, MOCK_HTTPDNS_SECRET);
    if (client == NULL) {
        goto cleanup;
    }
    // 3. 配置客户端（可选）
    hdns_client_set_timeout(client, 3000);
    hdns_client_set_using_cache(client, true);
    hdns_client_set_using_https(client, true);
    hdns_client_set_using_sign(client, true);
    hdns_client_set_retry_times(client, 1);
    hdns_client_set_region(client, "cn");

    // 4. 启动客户端
    hdns_client_start(client);
    // 5. 批量域名解析
    hdns_list_head_t *results = NULL;
    hdns_list_head_t *hosts = hdns_list_create();
    hdns_list_add_str(hosts, "www.taobao.com");
    hdns_list_add_str(hosts, "www.aliyun.com");
    hdns_list_add_str(hosts, "www.tmall.com");

    hdns_status_t status = hdns_get_results_for_hosts_sync_with_cache(client,
                                                                      hosts,
                                                                      HDNS_QUERY_IPV4,
                                                                      NULL,
                                                                      &results);
    // 6. 获取解析结果
    if (hdns_status_is_ok(&status)) {
        hdns_list_for_each_entry_safe(cursor, results) {
            hdns_resv_resp_t *resp = cursor->data;
            printf("%s==>", resp->host);
            hdns_list_for_each_entry_safe(ip_cursor, resp->ips) {
                printf("%s", (char *) ip_cursor->data);
                if (!hdns_list_is_end_node(ip_cursor, resp->ips)) {
                    printf("%s", ",");
                }
            }
            printf("\n");
        }
    } else {
        fprintf(stderr, "resv failed, error_code %s, error_msg:%s", status.error_code, status.error_msg);
    }
    // 7. 清理请求和结果
    hdns_list_cleanup(results);
    hdns_list_cleanup(hosts);
    // 8. 客户端释放
    hdns_client_cleanup(client);
    cleanup:
    // 9. SDK环境释放
    hdns_sdk_cleanup();
    printf("OK, Exit.\n");
    return 0;
}
