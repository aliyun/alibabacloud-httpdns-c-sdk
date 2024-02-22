# SDK简介

阿里云EMAS提供HTTPDNS C SDK，以帮助开发者降低在嵌入式、Linux、Windows、Mac等平台下接入HTTPDNS的门槛（目前仅适配了Linux平台）。

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

目前只适配了Mac M1、Linux平台，暂不支持Windows、Android、IOS、RTOS等平台。

# 安装方法

## 环境依赖
HTTPDNS C SDK依赖于openssl、libcurl、cjson、check等库，在构建之前确保本机上已经按照了这些环境，如果没有可以参考以下步骤进行安装。

### openssl下载及安装

HTTPDNS C SDK使用openssl完成自定义https证书校验，您需要确认这些库已经安装，并且将它们的头文件目录和库文件目录都加入到了项目中。

* 源码安装
  - 参考[openssl源码安装](https://github.com/openssl/openssl/blob/master/INSTALL.md)
* 二进制安装
  - macOS: ```brew install openssl```
  - Ubuntu/Debian:```sudo apt-get install libssl-dev```
  - Red Hat/CentOS/Fedora:```sudo yum install openssl-devel```
  - [Windows安装](https://slproweb.com/products/Win32OpenSSL.html)

### libcurl下载及安装

HTTPDNS C SDK使用curl进行网络操作，您需要确认这些库已经安装，并且将它们的头文件目录和库文件目录都加入到了项目中。

* 源码安装
  - [这里](http://curl.haxx.se/download.html)下载，并参考[libcurl 安装指南](http://curl.haxx.se/docs/install.html)

* 二进制安装
  - macOS: ```brew install curl```
  - Ubuntu/Debian:```sudo apt-get install libcurl4-openssl-dev```
  - Red Hat/CentOS/Fedora:```sudo yum install libcurl-devel```
  - [Windows安装](https://curl.se/windows/)

### cjson下载及安装

HTTPDNS C SDK使用cjson进行响应报文的解析，您需要确认这些库已经安装，并且将它们的头文件目录和库文件目录都加入到了项目中。
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

### check下载及安装

HTTPDNS C SDK使用check框架作为自己的单元测试框架，该框架的下载及安装如下：

```shell
  git clone https://github.com/libcheck/check.git
  cd check/
  mkdir build
  cd build/
  cmake ..
  make
  sudo make install
```

## HTTPDNS C SDK的安装使用
### 下载
您可以使用如下命令获取代码：

```shell
git clone git@gitlab.alibaba-inc.com:alicloud-ams/alicloud-httpdns-sdk-c.git
```
### 安装
安装时请在cmake命令中指定第三方库头文件以及库文件的路径，典型的编译命令如下：

```shell
    mkdir build
    cd build
    cmake  -DHTTPDNS_LOG_LEVEL=HTTPDNS_LOG_INFO  -DHTTPDNS_LOG_FILE_PATH=/tmp/httpdns.log  -DHTTPDNS_REGION=cn  -DHTTPDNS_RETRY_TIMES=2  ../
    make
    make test
    sudo make install
```

* 注意：
  - 这里假设工程所在的构建机器已经具有了C/C++的编译器、cmake环境
  - 执行cmake . 时默认会到/usr/local/下面去寻找curl的头文件和库文件。
  - 默认编译是Release类型，可以指定以下几种编译类型： Debug, Release,
    RelWithDebInfo和MinSizeRel，如果要使用Debug类型编译，则执行cmake . -DCMAKE_BUILD_TYPE=Debug
  - 如果您在安装curl时指定了安装目录，则需要在执行cmake时指定这些库的路径，比如：```shell cmake . -DCURL_INCLUDE_DIR=/usr/local/include/curl/ -DCURL_LIBRARY=/usr/local/lib/libcurl.a ```
  - 如果要指定安装目录，则需要在cmake时增加： -DCMAKE_INSTALL_PREFIX=/your/install/path/usr/local/
  - 可选构建参数如下：

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
# 添加HTTPDNS 头文件安装位置
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
核心头文件是```httpdns_client_wrapper.h```，具体使用步骤参考examples文件夹下的使用示例。

#### 运行示例程序
编译运行示例程序前，需要确认本地已经安装了C++编译器。
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
SDK提供了同步接口，默认超时时间为2500ms，当HTTPDNS部分服务IP发生异常时，可能会因为解析超时而导致的业务阻塞卡顿，所以可以根据业务的实际情况通过以下代码进行配置自定义配置
```c
   httpdns_config_t *httpdns_config = httpdns_client_get_config();
   httpdns_config_set_timeout_ms(httpdns_config, 1000);
```


