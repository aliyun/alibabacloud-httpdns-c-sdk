# SDK简介

阿里云移动研发平台EMAS提供HTTPDNS C SDK，以降低开发者在嵌入式、Linux、Windows、Mac等非Android/IOS平台下接入[HTTPDNS](https://www.aliyun.com/product/httpdns)的门槛（目前仅适配了Linux平台）。

## SDK 特点

* 高可用
  - 内置多组启动IP
  - 解析失败自动重试
* 性能
  - 支持异步解析
  - 支持http2协议
  - 支持本地缓存
  - 支持批量解析
  - 支持预解析
* 安全
  - 支持https
  - 请求加签防盗刷
* 解析类型
  - 域名A记录或AAAA记录
  - 支持根据网络类型自行解析对应的记录类型
* 易用
  - 屏蔽https ip证书校验等细节
  - 提供示例Example程序

## SDK限制

目前只适配了Linux平台，暂不支持Windows、Android、IOS、RTOS等平台。

# 安装方法

## 环境依赖
SDK的编译依赖于openssl、libcurl、cjson、check等库，在构建之前请确保本机已经安装了这些库。

* 注意：这里默认构建机器上已经安装了cmake、gcc、git等编译构建工具

### openssl安装

* 源码安装
  - 参考[openssl源码安装](https://github.com/openssl/openssl/blob/master/INSTALL.md)
* 二进制安装
  - macOS: ```brew install openssl```
  - Ubuntu/Debian:```sudo apt-get install libssl-dev```
  - Red Hat/CentOS/Fedora:```sudo yum install openssl-devel```
  - [Windows安装](https://slproweb.com/products/Win32OpenSSL.html)

### libcurl安装

* 源码安装
  - [这里](http://curl.haxx.se/download.html)下载，并参考[libcurl 安装指南](http://curl.haxx.se/docs/install.html)

* 二进制安装
  - macOS: ```brew install curl```
  - Ubuntu/Debian:```sudo apt-get install libcurl4-openssl-dev```
  - Red Hat/CentOS/Fedora:```sudo yum install libcurl-devel```
  - [Windows安装](https://curl.se/windows/)

### cjson安装

* 源码安装(推荐)
```shell
  git clone https://github.com/DaveGamble/cJSON.git
  cd cJSON
  mkdir build
  cd build
  cmake .. -DENABLE_CJSON_UTILS=On -DENABLE_CJSON_TEST=Off 
  make
  sudo make install
```
* 二进制安装
  - Ubuntu/Debian:```sudo apt-get install libcjson1 libcjson-dev```
  - Red Hat/CentOS/Fedora:```sudo yum install libcjson libcjson-devel```

### check安装

```shell
  git clone https://github.com/libcheck/check.git
  cd check/
  mkdir build
  cd build/
  cmake ..
  make
  sudo make install
```

## SDK的安装使用
### 安装
通过git clone获取代码后通过以下命令进行安装：

```shell
    mkdir build
    cd build
    cmake  -DHTTPDNS_LOG_LEVEL=HTTPDNS_LOG_INFO  -DHTTPDNS_LOG_FILE_PATH=/tmp/httpdns.log  -DHTTPDNS_REGION=cn  -DHTTPDNS_RETRY_TIMES=2  ../
    make
    make test
    sudo make install
```

* 可选构建参数如下：

| 参数                    | 说明          | 取值                                                                                                                            |
|-----------------------|-------------|-------------------------------------------------------------------------------------------------------------------------------|
| HTTPDNS_LOG_LEVEL     | 日志打印级别      | HTTPDNS_LOG_TRACE<br/>HTTPDNS_LOG_DEBUG<br/>HTTPDNS_LOG_INFO<br/>HTTPDNS_LOG_WARN<br/>HTTPDNS_LOG_ERROR<br/>HTTPDNS_LOG_FATAL |
| HTTPDNS_LOG_FILE_PATH | 日志文件存储路径    | 文件路径，路径长度最长不超过1023                                                                                                            |
| HTTPDNS_REGION        | HTTPDNS服务集群 | 中国大陆：cn<br/>海外香港：hk<br/>海外新加坡：sg                                                                                              |
| HTTPDNS_RETRY_TIMES   | 解析失败后的重试次数  | 0~5的整数，重试次数太多会导致接口调用耗时较长                                                                                                      |

### 使用

#### 构建配置

集成HTTPDNS C SDK构建应用，需要在CMakeLists.txt构建文件添加以指令：
```cmake
# 应用名和源文件
SET(APPLICATION_BIN_NAME httpdns_test_demo)
aux_source_directory(${CMAKE_SOURCE_DIR} SOURCE_FILES)
# 添加HTTPDNS头文件安装位置
SET(HTTPDNS_INCLUDE_HEADER /usr/local/include/)
include_directories(${HTTPDNS_INCLUDE_HEADER})
# 添加可执行目标应用
add_executable(${APPLICATION_BIN_NAME} ${SOURCE_FILES})
# 链接HTTPDNS静态库
find_library(HTTPDNS_LIBRARY httpdns_c_sdk_static)
target_link_libraries(${APPLICATION_BIN_NAME} ${HTTPDNS_LIBRARY})
# 链接libcurl网络库
find_library(CURL_LIBRARY curl)
target_link_libraries(${APPLICATION_BIN_NAME} ${CURL_LIBRARY})
# 链接pthread线程库
find_library(PTHREAD_LIBRARY pthread)
target_link_libraries(${APPLICATION_BIN_NAME} ${PTHREAD_LIBRARY})
# 链接openssl安全库
find_library(SSL_LIBRARY ssl)
find_library(CRYPTO_LIBRARY crypto)
target_link_libraries(${APPLICATION_BIN_NAME} ${SSL_LIBRARY})
target_link_libraries(${APPLICATION_BIN_NAME} ${CRYPTO_LIBRARY})
# 链接cjson库
find_library(CJSON_LIBRARY cjson)
target_link_libraries(${APPLICATION_BIN_NAME} ${CJSON_LIBRARY})

```
#### 代码使用
核心头文件是```httpdns_client_config.h```,```httpdns_client_wrapper.h```，前者是HTTPDNS客户端的配置接口，后者是HTTPDNS的解析接口，具体使用步骤参考examples文件夹下的使用示例。

#### 运行示例程序
```shell
cd   alicloud-httpdns-sdk-c/examples
mkdir  build
cd build
cmake  ../
make 
# C语言客户端应用集成示例，同步解析并访问www.aliyun.com网站
./build/bin/sync_client_example
# C语言客户端应用集成示例，异步解析并访问www.aliyun.com网站
./build/bin/async_client_example
# C语言服务器应用集成示例，同步解析，并打印解析结果
./build/bin/sync_server_example
# C++语言客户端应用集成示例，同步解析并访问www.aliyun.com网站
./build/bin/sync_client_cxx_example

```
#### 风险提示
SDK提供了同步接口，默认超时时间为2500ms，当HTTPDNS部分服务IP发生异常时，可能会因为解析超时而导致的业务阻塞卡顿，所以可以根据业务的实际情况通过以下代码进行自定义配置
```c
   httpdns_config_t *httpdns_config = httpdns_client_get_config();
   httpdns_config_set_timeout_ms(httpdns_config, 1000);
```


