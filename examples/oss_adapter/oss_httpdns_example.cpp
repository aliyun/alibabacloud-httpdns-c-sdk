/**
 * @file oss_httpdns_example.cpp
 * @brief 演示如何在阿里云 OSS C++ SDK 中集成 HTTPDNS
 * 
 * 环境变量：
 * - OSS_ACCESS_KEY_ID: OSS AccessKey ID
 * - OSS_ACCESS_KEY_SECRET: OSS AccessKey Secret
 * - OSS_ENDPOINT: OSS Endpoint (如 oss-cn-hangzhou.aliyuncs.com)
 * - HTTPDNS_ACCOUNT_ID: HTTPDNS 账户 ID
 * - HTTPDNS_SECRET_KEY: HTTPDNS 密钥（可选）
 */

#include <iostream>
#include <cstdlib>
#include <memory>

// HTTPDNS SDK
extern "C" {
#include "hdns_api.h"
}

// OSS SDK
#include <alibabacloud/oss/OssClient.h>

// HTTPDNS-OSS 适配器
#include "hdns_oss_interceptor.h"

using namespace AlibabaCloud::OSS;

static std::string getEnvOrDefault(const char *name, const char *defaultValue = "") {
    const char *value = std::getenv(name);
    return value ? value : defaultValue;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    std::string accessKeyId = getEnvOrDefault("OSS_ACCESS_KEY_ID");
    std::string accessKeySecret = getEnvOrDefault("OSS_ACCESS_KEY_SECRET");
    std::string endpoint = getEnvOrDefault("OSS_ENDPOINT");
    std::string hdnsAccountId = getEnvOrDefault("HTTPDNS_ACCOUNT_ID");
    std::string hdnsSecretKey = getEnvOrDefault("HTTPDNS_SECRET_KEY");

    if (accessKeyId.empty() || accessKeySecret.empty() || endpoint.empty()) {
        std::cerr << "错误: 请设置环境变量 OSS_ACCESS_KEY_ID, OSS_ACCESS_KEY_SECRET, OSS_ENDPOINT" << std::endl;
        return 1;
    }

    if (hdnsAccountId.empty()) {
        std::cerr << "错误: 请设置环境变量 HTTPDNS_ACCOUNT_ID" << std::endl;
        return 1;
    }

    std::cout << "=== OSS + HTTPDNS 集成示例 ===" << std::endl;
    std::cout << "Endpoint: " << endpoint << std::endl;
    std::cout << "HTTPDNS Account: " << hdnsAccountId << std::endl;

    // 初始化 HTTPDNS SDK
    std::cout << "\n[1] 初始化 HTTPDNS SDK..." << std::endl;
    if (hdns_sdk_init() != HDNS_OK) {
        std::cerr << "HTTPDNS SDK 初始化失败" << std::endl;
        return 1;
    }

    // 创建 HTTPDNS 客户端
    std::cout << "[2] 创建 HTTPDNS 客户端..." << std::endl;
    const char *secretKey = hdnsSecretKey.empty() ? nullptr : hdnsSecretKey.c_str();
    hdns_client_t *hdnsClient = hdns_client_create(hdnsAccountId.c_str(), secretKey);
    if (!hdnsClient) {
        std::cerr << "HTTPDNS 客户端创建失败" << std::endl;
        hdns_sdk_cleanup();
        return 1;
    }

    // 配置 HTTPDNS 客户端
    hdns_client_set_timeout(hdnsClient, 2000);                              // 请求超时时间，单位：ms
    hdns_client_set_using_cache(hdnsClient, true);                          // 开启本地缓存
    hdns_client_set_using_https(hdnsClient, true);                          // 使用 HTTPS 访问 HTTPDNS 服务端
    hdns_client_set_using_sign(hdnsClient, true);                           // 请求加签
    hdns_client_set_retry_times(hdnsClient, 1);                             // 重试次数
    hdns_client_set_region(hdnsClient, "global");                           // HTTPDNS 解析服务集群
    hdns_client_set_schedule_center_region(hdnsClient, "cn");               // HTTPDNS 调度集群
    hdns_client_enable_update_cache_after_net_change(hdnsClient, true);     // 网络变化后更新缓存
    hdns_client_enable_expired_ip(hdnsClient, true);                        // 允许获取过期缓存
    hdns_client_enable_failover_localdns(hdnsClient, true);                 // 自动降级到 LocalDNS
    hdns_client_add_pre_resolve_host(hdnsClient, endpoint.c_str());         // 预解析域名
    hdns_client_add_ip_probe_item(hdnsClient, endpoint.c_str(), 443);       // IP 嗅探

    // 启动 HTTPDNS 客户端
    hdns_status_t hdnsStatus = hdns_client_start(hdnsClient);
    if (!hdns_status_is_ok(&hdnsStatus)) {
        std::cerr << "HTTPDNS 客户端启动失败: " << hdnsStatus.error_msg << std::endl;
        hdns_client_cleanup(hdnsClient);
        hdns_sdk_cleanup();
        return 1;
    }
    std::cout << "   HTTPDNS 客户端启动成功" << std::endl;

    // 初始化 OSS SDK
    std::cout << "[3] 初始化 OSS SDK..." << std::endl;
    InitializeSdk();

    // 创建 HTTPDNS 拦截器
    std::cout << "[4] 创建 HTTPDNS-OSS 适配器..." << std::endl;
    auto hdnsInterceptor = std::make_shared<AlibabaCloud::HDNS::HdnsOssInterceptor>(
        hdnsClient, HDNS_QUERY_AUTO);

    // 配置 OSS 客户端
    std::cout << "[5] 配置 OSS 客户端..." << std::endl;
    ClientConfiguration conf;
    conf.httpInterceptor = hdnsInterceptor;

    // 测试 HTTP 模式
    std::cout << "[6] 测试 HTTP 模式..." << std::endl;
    {
        OssClient ossClient("http://" + endpoint, accessKeyId, accessKeySecret, conf);
        ListBucketsRequest request;
        auto outcome = ossClient.ListBuckets(request);
        if (outcome.isSuccess()) {
            std::cout << "   ✓ HTTP 模式成功" << std::endl;
        } else {
            std::cerr << "   ✗ HTTP 模式失败: " << outcome.error().Message() << std::endl;
            ShutdownSdk();
            hdns_client_cleanup(hdnsClient);
            hdns_sdk_cleanup();
            return 1;
        }
    }

    // 测试 HTTPS 模式
    std::cout << "[7] 测试 HTTPS 模式..." << std::endl;
    {
        OssClient ossClient("https://" + endpoint, accessKeyId, accessKeySecret, conf);
        ListBucketsRequest request;
        auto outcome = ossClient.ListBuckets(request);
        if (outcome.isSuccess()) {
            std::cout << "   ✓ HTTPS 模式成功" << std::endl;
            std::cout << "\n=== Bucket 列表 ===" << std::endl;
            for (const auto &bucket : outcome.result().Buckets()) {
                std::cout << "  - " << bucket.Name() 
                          << " (Location: " << bucket.Location() << ")" << std::endl;
            }
        } else {
            std::cerr << "   ✗ HTTPS 模式失败: " << outcome.error().Message() << std::endl;
            ShutdownSdk();
            hdns_client_cleanup(hdnsClient);
            hdns_sdk_cleanup();
            return 1;
        }
    }

    std::cout << "\n✓ HTTP 和 HTTPS 模式均成功，HTTPDNS 集成正常工作！" << std::endl;

    // 清理资源
    std::cout << "\n[8] 清理资源..." << std::endl;
    ShutdownSdk();
    hdns_client_cleanup(hdnsClient);
    hdns_sdk_cleanup();

    std::cout << "完成！" << std::endl;
    return 0;
}
