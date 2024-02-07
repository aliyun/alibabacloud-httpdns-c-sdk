//
// Created by caogaoshuai on 2024/2/7.
//
#include "httpdns_client_wrapper.h"
#include "httpdns_log.h"
#include "httpdns_time.h"
#include "httpdns_localdns.h"
#include <curl/curl.h>

#define MOCK_BUSINESS_HOST   "www.aliyun.com"
#define MOCK_HTTPDNS_ACCOUNT   "139450"
#define MOCK_ASYNC_REQUEST_NUM    100

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
    curl = curl_easy_init();
    if (curl) {
        // 4.1 拼接业务URL
        sds url = sdsnew("https://");
        url = sdscat(url, MOCK_BUSINESS_HOST);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);

        // 4.2 HTTPS设置预解析的主机和 IP
        struct curl_slist *dns;
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
            fprintf(stderr, "curl_easy_perform() failed, url=%s, ip=%s, error=%s\n",
                    url,
                    dst_ip,
                    curl_easy_strerror(res));
        }
        // 4.5 释放业务访问相关资源
        sdsfree(url);
        sdsfree(resolve_param);
        curl_slist_free_all(dns);
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
}

// 3. 构建解析回调函数
static void httpdns_complete_callback_func(const httpdns_resolve_result_t *result,
                                           void *user_callback_param) {

    if (NULL == result) {
        log_trace("httpdns resolve failed, fallback to localdns");
        result = resolve_host_by_localdns(MOCK_BUSINESS_HOST);
        sds localdns_result_str = httpdns_resolve_result_to_string(result);
        printf("localdns reuslt %s\n", localdns_result_str);
        sdsfree(localdns_result_str);
        httpdns_resolve_result_free(result);
        return;
    }
    if (IS_NOT_EMPTY_LIST(&result->ips) || IS_NOT_EMPTY_LIST(&result->ipsv6)) {
        char dst_ip[INET6_ADDRSTRLEN];
        select_ip_from_httpdns_result(result, dst_ip);
        mock_access_business_web_server(dst_ip);
        int32_t *success_num = user_callback_param;
        *success_num = *success_num + 1;
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
    int32_t success_num = 0;
    // 2. 异步提交多个解析任务
    struct timeval start_time = httpdns_time_now();
    for (int i = 0; i < MOCK_ASYNC_REQUEST_NUM; i++) {
        get_httpdns_result_for_host_async_with_cache(MOCK_BUSINESS_HOST,
                                                     HTTPDNS_QUERY_TYPE_AUTO,
                                                     NULL,
                                                     httpdns_complete_callback_func,
                                                     &success_num);
    }
    // 4. 等待结果完成
    sleep(30);
    struct timeval end_time = httpdns_time_now();
    log_trace("async client in linux example, access business cost %ld ms/次, success number %d",
              httpdns_time_diff(end_time, start_time) / MOCK_ASYNC_REQUEST_NUM, success_num);
    // 5. HTTPDNS SDK 环境释放
    httpdns_client_env_cleanup();
    httpdns_log_stop();
    return 0;
}
