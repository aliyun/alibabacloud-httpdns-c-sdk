# OSS SDK HTTPDNS 适配器

本目录提供了阿里云 OSS C++ SDK 与 HTTPDNS C SDK 的集成适配器。

## 文件说明

- `hdns_oss_interceptor.h` - Header-only 适配器，实现 OSS SDK 的 `HttpInterceptor` 接口
- `oss_httpdns_example.cpp` - 完整的使用示例

## 工作原理

OSS C++ SDK 提供了 `HttpInterceptor` 扩展点，允许在 HTTP 请求发送前进行拦截处理。
适配器通过以下步骤实现 HTTPDNS 集成：

1. 在 `preSendRequest` 回调中获取请求的目标域名
2. 调用 HTTPDNS SDK 解析域名获取 IP
3. 通过 `CURLOPT_RESOLVE` 将解析结果注入到 libcurl

## 使用方法

### 1. 编译两个 SDK

分别编译 HTTPDNS C SDK 和 OSS C++ SDK：

```bash
# 编译 HTTPDNS C SDK
cd alibabacloud-httpdns-c-sdk
mkdir build && cd build
cmake ..
make

# 编译 OSS C++ SDK (参考 OSS SDK 文档)
cd aliyun-oss-cpp-sdk
mkdir build && cd build
cmake ..
make
```

### 2. 在项目中集成

```cpp
#include "hdns_oss_interceptor.h"
#include <alibabacloud/oss/OssClient.h>

// 初始化 HTTPDNS
hdns_sdk_init();
hdns_client_t *hdnsClient = hdns_client_create("your_account_id", "your_secret");
hdns_client_start(hdnsClient);

// 创建适配器
auto interceptor = std::make_shared<AlibabaCloud::HDNS::HdnsOssInterceptor>(hdnsClient);

// 配置 OSS 客户端
AlibabaCloud::OSS::ClientConfiguration conf;
conf.httpInterceptor = interceptor;  // 关键配置

AlibabaCloud::OSS::OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

// 正常使用 OSS 客户端，DNS 解析会自动走 HTTPDNS
auto outcome = client.ListBuckets();
```

### 3. 编译示例

```bash
cd alibabacloud-httpdns-c-sdk/build
cmake .. \
  -DOSS_SDK_INCLUDE_DIR=/path/to/oss-sdk/include \
  -DOSS_SDK_LIBRARIES=/path/to/libalibabacloud-oss-cpp-sdk.a
make oss_httpdns_example
```

### 4. 运行示例

```bash
export OSS_ACCESS_KEY_ID="your_access_key_id"
export OSS_ACCESS_KEY_SECRET="your_access_key_secret"
export OSS_ENDPOINT="oss-cn-hangzhou.aliyuncs.com"
export HTTPDNS_ACCOUNT_ID="your_httpdns_account"
export HTTPDNS_SECRET_KEY="your_httpdns_secret"  # 可选

./oss_httpdns_example
```

## 注意事项

1. **生命周期管理**：HTTPDNS 客户端的生命周期需要长于 OSS 客户端
2. **线程安全**：`HdnsOssInterceptor` 内部会管理 `curl_slist` 资源，每次请求前会清理上次的资源
3. **错误处理**：如果 HTTPDNS 解析失败，会自动降级到 libcurl 默认的 DNS 解析
4. **IPv6 支持**：可通过构造函数的 `queryType` 参数指定 `HDNS_QUERY_IPV6` 或 `HDNS_QUERY_BOTH`
