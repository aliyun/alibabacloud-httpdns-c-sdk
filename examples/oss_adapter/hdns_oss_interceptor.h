/**
 * @file hdns_oss_interceptor.h
 * @brief HTTPDNS 与阿里云 OSS C++ SDK 的适配器
 * 
 * 本文件提供了一个 HttpInterceptor 实现，用于将 HTTPDNS 解析结果
 * 注入到 OSS SDK 的 HTTP 请求中，实现 DNS 防劫持和加速。
 * 
 * 使用方式：
 * 1. 初始化 HTTPDNS SDK 并创建客户端
 * 2. 创建 HdnsOssInterceptor 实例
 * 3. 将其设置到 OSS ClientConfiguration 的 httpInterceptor 字段
 * 
 * 可定制点（可根据业务需求修改源码）：
 * - 解析方式：当前使用 hdns_get_result_for_host_sync_with_cache（同步+缓存）
 *            可替换为 _without_cache 或异步 API
 * - IP选择策略：当前使用 hdns_select_ip_randomly（随机选择）
 *              可替换为 hdns_select_first_ip 或自定义策略
 * - 查询类型：构造函数默认 HDNS_QUERY_AUTO，可固定为 IPv4/IPv6
 * 
 * 注意：清理顺序需先释放 OSS 客户端，再释放 HTTPDNS 客户端
 */

#ifndef HDNS_OSS_INTERCEPTOR_H
#define HDNS_OSS_INTERCEPTOR_H

#include <string>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <curl/curl.h>
#include <alibabacloud/oss/http/HttpInterceptor.h>
#include <alibabacloud/oss/http/HttpRequest.h>

extern "C" {
#include "hdns_api.h"
}

namespace AlibabaCloud {
namespace HDNS {

/**
 * @class HdnsOssInterceptor
 * @brief OSS SDK 的 HTTPDNS 拦截器
 * 
 * 通过实现 OSS SDK 的 HttpInterceptor 接口，在请求发送前
 * 使用 HTTPDNS 解析域名，并通过 CURLOPT_RESOLVE 注入解析结果
 */
class HdnsOssInterceptor : public OSS::HttpInterceptor {
public:
    /**
     * @brief 构造函数
     * @param client HTTPDNS 客户端实例（需要在外部管理生命周期）
     * @param queryType DNS 查询类型，默认自动选择
     */
    explicit HdnsOssInterceptor(hdns_client_t *client, 
                                 hdns_query_type_t queryType = HDNS_QUERY_AUTO)
        : hdnsClient_(client)
        , queryType_(queryType) {
    }

    ~HdnsOssInterceptor() override {
        // 清理所有残留的 resolve list
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto &pair : resolveListMap_) {
            if (pair.second) {
                curl_slist_free_all(pair.second);
            }
        }
        resolveListMap_.clear();
    }

    /**
     * @brief 请求发送前的拦截处理
     * @param handler curl handle (CURL*)
     * @param request HTTP 请求对象
     */
    void preSendRequest(void *handler, 
                        const std::shared_ptr<OSS::HttpRequest> &request) override {
        if (!handler || !request || !hdnsClient_) {
            return;
        }

        CURL *curl = static_cast<CURL*>(handler);
        
        // 清理该 curl handle 上一次的 resolve list（如果有）
        cleanupForHandle(curl);

        // 获取请求的 host 和 port
        std::string host = request->url().host();
        int port = request->url().port();
        std::string scheme = request->url().scheme();
        
        // 确定端口号
        if (port <= 0) {
            port = (scheme == "https") ? 443 : 80;
        }

        // 使用 HTTPDNS 解析域名
        hdns_list_head_t *results = nullptr;
        hdns_status_t status = hdns_get_result_for_host_sync_with_cache(
            hdnsClient_, host.c_str(), queryType_, nullptr, &results);

        if (!hdns_status_is_ok(&status) || !results) {
            // 解析失败，让 libcurl 使用默认 DNS
            hdns_log_info("HTTPDNS resolve failed for host=%s, fallback to system DNS", host.c_str());
            if (results) {
                hdns_list_cleanup(results);
            }
            return;
        }

        // 获取解析到的 IP（随机选择，实现负载均衡）
        char ip[HDNS_IP_ADDRESS_STRING_LENGTH] = {0};
        if (hdns_select_ip_randomly(results, queryType_, ip) != HDNS_OK) {
            hdns_list_cleanup(results);
            return;
        }

        // 构建 CURLOPT_RESOLVE 格式: "host:port:ip"
        char resolveEntry[512];
        snprintf(resolveEntry, sizeof(resolveEntry), "%s:%d:%s", 
                 host.c_str(), port, ip);

        // 创建新的 resolve list
        struct curl_slist *resolveList = curl_slist_append(nullptr, resolveEntry);
        if (resolveList) {
            curl_easy_setopt(curl, CURLOPT_RESOLVE, resolveList);
            
            // 存储到 map 中，以便后续释放
            std::lock_guard<std::mutex> lock(mutex_);
            resolveListMap_[curl] = resolveList;
        }

        // 清理 HTTPDNS 解析结果
        hdns_list_cleanup(results);
    }

private:
    void cleanupForHandle(CURL *curl) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = resolveListMap_.find(curl);
        if (it != resolveListMap_.end()) {
            if (it->second) {
                curl_slist_free_all(it->second);
            }
            resolveListMap_.erase(it);
        }
    }

    hdns_client_t *hdnsClient_;
    hdns_query_type_t queryType_;
    
    // 使用 mutex 保护的 map，按 curl handle 存储 resolve list
    std::mutex mutex_;
    std::unordered_map<CURL*, struct curl_slist*> resolveListMap_;
};

} // namespace HDNS
} // namespace AlibabaCloud

#endif // HDNS_OSS_INTERCEPTOR_H
