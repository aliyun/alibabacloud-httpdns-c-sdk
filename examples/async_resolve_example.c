//
// Created by caogaoshuai on 2024/2/7.
//

#include "hdns_api.h"
#include <curl/curl.h>

#define MOCK_BUSINESS_HOST        "www.taobao.com"
#define MOCK_HTTPDNS_ACCOUNT      "139450"
#define MOCK_HTTPDNS_SECRET       NULL


static size_t write_data_callback(void *buffer, size_t size, size_t nmemb, void *write_data) {
    hdns_to_void_p(buffer);
    hdns_to_void_p(write_data);
    size_t real_size = size * nmemb;
    printf("get %dB data\n", size * nmemb);
    return real_size;
}

// 8. 访问业务
static void mock_access_business_web_server(const char *dst_ip) {
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        // 8.1 拼接业务URL
        char url[256];
        strcpy(url, "https://");
        strcat(url, MOCK_BUSINESS_HOST);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);

        // 8.2 HTTPS设置预解析的主机和 IP
        struct curl_slist *dns;
        char sni[256];
        strcpy(sni, MOCK_BUSINESS_HOST);
        strcat(sni, ":443:");
        strcat(sni, dst_ip);
        dns = curl_slist_append(NULL, sni);
        curl_easy_setopt(curl, CURLOPT_RESOLVE, dns);
        // 8.3 设置响应结果回调
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_callback);
#if defined(_WIN32)
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif
        // 8.4 发起HTTP请求
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed, url=%s, ip=%s, error=%s\n",
                    url,
                    dst_ip,
                    curl_easy_strerror(res));
        }
        // 8.5 释放业务访问相关资源
        curl_slist_free_all(dns);
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
}

// 7. 构建解析结束回调函数
static void hdns_resv_done_callback(hdns_status_t *status, hdns_list_head_t *results, void *param) {
    bool *success = param;
    *success = hdns_status_is_ok(status);
    if (hdns_status_is_ok(status)) {
        hdns_list_for_each_entry_safe(cursor, results) {
            printf("resolve success, ips [ ");
            hdns_resv_resp_t *resp = cursor->data;
            hdns_list_for_each_entry_safe(ip_cursor, resp->ips) {
                printf("%s", (char *) ip_cursor->data);
                if (!hdns_list_is_end_node(ip_cursor, resp->ips)) {
                    printf("%s", ",");
                }
            }
            printf("]\n");
        }
    } else {
        fprintf(stderr, "resv failed, error_code %s, error_msg:%s", status->error_code, status->error_msg);
    }
    if (hdns_status_is_ok(status)) {
        char ip[HDNS_IP_ADDRESS_STRING_LENGTH];
        if (hdns_select_ip_randomly(results, HDNS_QUERY_AUTO, ip) == HDNS_OK) {
            mock_access_business_web_server(ip);
        }
    }
}


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

    // 5. 异步域名解析
    bool success = false;
    hdns_status_t status = hdns_get_result_for_host_async_with_cache(client,
                                                                     MOCK_BUSINESS_HOST,
                                                                     HDNS_QUERY_AUTO,
                                                                     NULL,
                                                                     hdns_resv_done_callback,
                                                                     &success);
    if (!hdns_status_is_ok(&status)) {
        goto cleanup;
    }
    // 6. 等待结果完成（正常业务可以不做等待，而是操作其他异步任务，这里仅为了演示）
    apr_sleep(1 * APR_USEC_PER_SEC);
    // 8. 客户端释放
    hdns_client_cleanup(client);
    cleanup:
    // 9. SDK环境释放
    hdns_sdk_cleanup();
    printf("OK, Exit.\n");
    return 0;
}
