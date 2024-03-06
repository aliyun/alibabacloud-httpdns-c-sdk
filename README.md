# Aliyun HTTPDNS SDK for C

[![GitHub version](https://badge.fury.io/gh/aliyun%2Falibabacloud-httpdns-c-sdk.git.svg)](https://badge.fury.io/gh/aliyun%2Falibabacloud-httpdns-c-sdk.git)
[![Software License](https://img.shields.io/badge/license-MIT-brightgreen.svg)](LICENSE)

## 关于
阿里云[HTTPDNS](https://www.aliyun.com/product/httpdns)是面向多端应用（移动端APP，PC客户端应用）具有防劫持、精准调度、实时解析生效等特性的域名解析服务。阿里云EMAS团队提供了HTTPDNS C SDK，以降低开发者在嵌入式、Linux、Windows、Mac等非Android/IOS平台下接入[HTTPDNS](https://www.aliyun.com/product/httpdns)的门槛（目前仅适配了Linux平台），用户可以通过调用相关API方便地使用HTTPDNS进行域名解析。

## SDK限制
目前只适配了Linux平台，暂不支持Windows、Android、IOS、RTOS等平台。

## 版本
- 当前版本：1.0.0

## 安装方法

### 环境依赖
HTTPDNS C SDK使用libcurl库进行网络操作，使用openssl库进行HTTPS的SSL层校验，HTTPDNS C SDK并没有带上这几个外部库，您需要确认这些库已经安装，并且将它们的头文件目录和库文件目录都加入到了项目中。

#### openssl安装

* 源码安装
  - 参考[openssl源码安装](https://github.com/openssl/openssl/blob/master/INSTALL.md)
* 二进制安装
  - macOS: ```brew install openssl```
  - Ubuntu/Debian:```sudo apt-get install libssl-dev```
  - Red Hat/CentOS/Fedora:```sudo yum install openssl-devel```
  - [Windows安装](https://slproweb.com/products/Win32OpenSSL.html)

#### libcurl安装

* 源码安装
  - [这里](http://curl.haxx.se/download.html)下载，并参考[libcurl 安装指南](http://curl.haxx.se/docs/install.html)

* 二进制安装
  - macOS: ```brew install curl```
  - Ubuntu/Debian:```sudo apt-get install libcurl4-openssl-dev```
  - Red Hat/CentOS/Fedora:```sudo yum install libcurl-devel```
  - [Windows安装](https://curl.se/windows/)

### SDK的安装使用
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


## License
- MIT

## 联系我们
- [阿里云HTTPDNS官方文档中心](https://www.aliyun.com/product/httpdns#Docs)
- 阿里云官方技术支持：[提交工单](https://workorder.console.aliyun.com/#/ticket/createIndex)
- 阿里云EMAS开发交流钉钉群：35248489