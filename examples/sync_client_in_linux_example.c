//
// Created by cagaoshuai on 2024/2/7.
//
#include "httpdns_client_wrapper.h"
#include "httpdns_log.h"
#include <curl/curl.h>

#define MOCK_BUSINESS_HOST   "www.aliyun.com"
#define MOCK_HTTPDNS_ACCOUNT   "139450"

static size_t write_data_callback(void *buffer, size_t size, size_t nmemb, void *write_data) {
    size_t real_size = size * nmemb;
    sds response_body = sdsnewlen(buffer, real_size);
    printf("%s", response_body);
    sdsfree(response_body);
    return real_size;
}


static void mock_access_business_web_server(const char *dst_ip) {
    CURL *curl;
    CURLcode res;
    struct curl_slist *dns;
    curl = curl_easy_init();
    if (curl) {
        // 4.1 拼接业务URL
        sds url = sdsnew("https://");
        url = sdscat(url, MOCK_BUSINESS_HOST);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

        // 4.2 HTTPS设置预解析的主机和 IP
        sds resolve_param = sdsnew(MOCK_BUSINESS_HOST);
        resolve_param = sdscat(resolve_param, ":443:");
        resolve_param = sdscat(resolve_param, dst_ip);
        dns = curl_slist_append(NULL, resolve_param);
        curl_easy_setopt(curl, CURLOPT_RESOLVE, dns);
        // 4.3 设置响应结果回调
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_callback);
        // 4.4 发起HTTP请求
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        // 4.5 释放业务访问相关资源
        sdsfree(url);
        sdsfree(resolve_param);
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
}


int main(int argc, char *argv[]) {
    // 1. HTTPDNS SDK 环境初始化
    httpdns_client_env_init(MOCK_HTTPDNS_ACCOUNT, NULL);
    httpdns_log_start();
    // 2. HTTPDNS SDK 解析结果
    httpdns_resolve_result_t *result = get_httpdns_result_for_host_sync_with_cache(MOCK_BUSINESS_HOST,
                                                                                   HTTPDNS_QUERY_TYPE_AUTO,
                                                                                   NULL);
    if (NULL == result || IS_EMPTY_LIST(&result->ips)) {
        perror("域名解析失败");
    } else {
        // 3. 根据网络类型获取相应的IP
        char dst_ip[INET6_ADDRSTRLEN];
        select_ip_from_httpdns_result(result, dst_ip);
        // 4. 使用解析的IP访问业务
        mock_access_business_web_server(dst_ip);
    }
    //5. 解析结果
    httpdns_resolve_result_free(result);
    //6. HTTPDNS SDK 环境释放
    httpdns_client_env_cleanup();
    httpdns_log_stop();
    return 0;
}
