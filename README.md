# SDK简介

阿里云EMAS提供HTTPDNS C SDK，以帮助开发者降低在嵌入式、Linux、Windows、Mac等场景下接入HTTPDNS的门槛。

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

### check下载及安装

HTTPDNS C SDK使用check框架作为自己的单元测试框架，该框架的下载及安装如下：

```shell
  git clone https://github.com/libcheck/check.git
  cd check/
  mkdir build
  cd build/
  cmake ..
  make
  make test
```

## HTTPDNS C SDK的安装使用
### 下载
您可以使用如下命令获取代码：

```shell
git clone https://code.alibaba-inc.com/alicloud-ams/alicloud-httpdns-sdk-c
```
### 安装
安装时请在cmake命令中指定第三方库头文件以及库文件的路径，典型的编译命令如下：

```shell
    mkdir build
    cd build
    cmake ..
    make
    ctest
    make install
```

* 注意：
  - 执行cmake . 时默认会到/usr/local/下面去寻找curl的头文件和库文件。
  - 默认编译是Release类型，可以指定以下几种编译类型： Debug, Release,
    RelWithDebInfo和MinSizeRel，如果要使用Debug类型编译，则执行cmake . -DCMAKE_BUILD_TYPE=Debug
  - 如果您在安装curl时指定了安装目录，则需要在执行cmake时指定这些库的路径，比如：
  ```shell
   cmake . -DCURL_INCLUDE_DIR=/usr/local/include/curl/ -DCURL_LIBRARY=/usr/local/lib/libcurl.a
  ```
  - 如果要指定安装目录，则需要在cmake时增加： -DCMAKE_INSTALL_PREFIX=/your/install/path/usr/local/

### 使用
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

```

