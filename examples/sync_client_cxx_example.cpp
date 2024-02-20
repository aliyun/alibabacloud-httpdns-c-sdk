//
// Created by caogaoshuai on 2024/2/7.
//
#include "httpdns/httpdns_client_wrapper.h"
#include "httpdns/httpdns_log.h"
#include "httpdns/httpdns_time.h"
#include "httpdns/httpdns_localdns.h"
#include <curl/curl.h>

#define MOCK_BUSINESS_HOST   "www.aliyun.com"
#define MOCK_HTTPDNS_ACCOUNT   "139450"

static size_t write_data_callback(void *buffer, size_t size, size_t nmemb, void *write_data) {
    size_t real_size = size * nmemb;
    httpdns_sds_t response_body = httpdns_sds_new_len(buffer, real_size);
    printf("%s", response_body);
    httpdns_sds_free(response_body);
    return real_size;
}


static void mock_access_business_web_server(const char *dst_ip) {
    CURL *curl;
    CURLcode res;
    struct curl_slist *dns;
    curl = curl_easy_init();
    if (curl) {
        // 5.1 拼接业务URL
        httpdns_sds_t url = httpdns_sds_new("https://");
        url = httpdns_sds_cat(url, MOCK_BUSINESS_HOST);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

        // 5.2 HTTPS设置预解析的主机和 IP
        httpdns_sds_t resolve_param = httpdns_sds_new(MOCK_BUSINESS_HOST);
        resolve_param = httpdns_sds_cat(resolve_param, ":443:");
        resolve_param = httpdns_sds_cat(resolve_param, dst_ip);
        dns = curl_slist_append(NULL, resolve_param);
        curl_easy_setopt(curl, CURLOPT_RESOLVE, dns);
        // 5.3 设置响应结果回调
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_callback);
        // 5.4 发起HTTP请求
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        // 5.5 释放业务访问相关资源
        httpdns_sds_free(url);
        httpdns_sds_free(resolve_param);
        curl_slist_free_all(dns);
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
}


int main(int argc, char *argv[]) {
    // 1. HTTPDNS SDK 环境初始化
    httpdns_client_env_init(MOCK_HTTPDNS_ACCOUNT, NULL);
      // 其他自定义配置
//    httpdns_config_t *httpdns_config = httpdns_client_get_config();
//    httpdns_config_set_using_https(httpdns_config, true);
//    httpdns_config_set_using_sign(httpdns_config, true);
//    httpdns_config_set_using_cache(httpdns_config, false);

    httpdns_log_start();
    // 2. HTTPDNS SDK 解析结果
    struct timeval start_time = httpdns_time_now();
    httpdns_resolve_result_t *result = get_httpdns_result_for_host_sync_with_cache(MOCK_BUSINESS_HOST,
                                                                                   HTTPDNS_QUERY_TYPE_AUTO,
                                                                                   NULL);
    struct timeval end_time = httpdns_time_now();
    log_trace("sync client in linux example, httpdns cost %ld ms", httpdns_time_diff(end_time, start_time));
    if (NULL == result || httpdns_list_is_empty(&result->ips)) {
        perror("域名解析失败");
    } else {
        // 4. 根据网络类型获取相应的IP
        char dst_ip[INET6_ADDRSTRLEN];
        select_ip_from_httpdns_result(result, dst_ip);
        // 5. 使用解析的IP访问业务
        start_time = httpdns_time_now();
        mock_access_business_web_server(dst_ip);
        end_time = httpdns_time_now();
        log_trace("sync client in linux example, access business cost %ld ms", httpdns_time_diff(end_time, start_time));

    }
    // 6. 解析结果释放
    httpdns_resolve_result_free(result);
    // 7. HTTPDNS SDK 环境释放
    httpdns_client_env_cleanup();
    httpdns_log_stop();
    return 0;
}
