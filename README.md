# Aliyun HTTPDNS SDK for C

[![GitHub version](https://badge.fury.io/gh/aliyun%2Falibabacloud-httpdns-c-sdk.git.svg)](https://badge.fury.io/gh/aliyun%2Falibabacloud-httpdns-c-sdk.git)
[![Software License](https://img.shields.io/badge/license-MIT-brightgreen.svg)](LICENSE)

## 关于

阿里云[HTTPDNS](https://www.aliyun.com/product/httpdns)
是面向多端应用（移动端APP，PC客户端应用）具有防劫持、精准调度、实时解析生效等特性的域名解析服务。阿里云EMAS团队提供了HTTPDNS C
SDK，以降低开发者在嵌入式、Linux、Windows、Mac等非Android/IOS平台下接入[HTTPDNS](https://www.aliyun.com/product/httpdns)
的门槛（目前仅适配了Linux平台），用户可以通过调用相关API方便地使用HTTPDNS进行域名解析。

## SDK限制

目前只适配了Linux平台，暂不支持Windows、Android、IOS、RTOS等平台。

## 版本

- 当前版本：2.0.0

## 安装方法

### 构建工具安装

构建过程中需要使用git克隆代码、使用cmake构建工程(版本3.0及以上)、使用gcc(版本4.5及以上)
/g++编译代码，请您确认这些命令行工具已经在安装本机，如果尚未安装参考以下安装命令：

- Ubuntu/Debian:

```shell
sudo  apt update
sudo  apt install -y git cmake gcc g++
```

- Aliyun/CentOS Stream/Fedora:

```shell
sudo yum check-update
sudo yum install git cmake  gcc  gcc-c++
```

- OpenSUSE:

```shell
sudo zypper refresh
sudo zypper install git cmake  gcc  gcc-c++
```

### 依赖库安装

HTTPDNS C SDK使用libcurl库(版本7.33.0及以上)进行网络操作，使用openssl库(版本1.1.0及以上)进行HTTPS的SSL层校验，使用apr/apr-util(版本1.5.2及以上)库解决内存管理以及跨平台问题，HTTPDNS C
SDK并没有带上这几个外部库，您需要确认这些库已经安装，并且将它们的头文件目录和库文件目录都加入到了项目中。

#### 二进制安装

- Ubuntu/Debian:

```shell
sudo apt install -y libssl-dev libcurl4-openssl-dev libapr1-dev libaprutil1-dev
```

- Aliyun/CentOS Stream/Fedora:

```shell
sudo yum check-update
sudo yum install openssl-devel libcurl-devel apr-util apr-devel apr-util-devel
```

- OpenSUSE:

```shell
sudo zypper refresh
sudo zypper install libopenssl-devel libcurl-devel libapr1-devel libapr-util1-devel 
```

<span style="color:red;">
注意：libcurl必须是依赖OpenSSL完成的SSL层通信，否则可能会导致程序异常，可以通过以下命令行检验当前libcurl是否使用了OpenSSL：</span>

```shell
curl  --version | grep -i -o  OpenSSL
```

#### 源码安装
##### libcurl （建议 7.33.0 及以上版本）
如果通过包管理器安装的libcurl库中没有使用OpenSSL库，那么需要[下载源码](https://curl.se/download/)
安装使用OpenSSL作为SSL层的libcurl库，这里以安装curl-7.61.0为例，步骤如下：

```shell
wget https://curl.se/download/curl-7.61.0.tar.gz
tar -xzvf curl-7.61.0.tar.gz
cd curl-7.61.0
./configure --with-ssl
make
sudo make install
sudo ldconfig
```

注意: 这里默认客户已安装OpenSSL库。


##### apr （建议 1.5.2 及以上版本）

请从[这里](https://apr.apache.org/download.cgi)下载，典型的安装方式如下：
 ```shell
    ./configure
    make
    make install
```

注意：
- 执行./configure时默认是配置安装目录为/usr/local/，如果需要指定安装目录，请使用 ./configure --prefix=/your/install/path/

##### apr-util （建议 1.5.4 及以上版本）

请从[这里](https://apr.apache.org/download.cgi)下载，安装时需要注意指定--with-apr选项，典型的安装方式如下：
```shell
    ./configure --with-apr=/your/apr/install/path
    make
    make install
```

注意：
- 执行./configure时默认是配置安装目录为/usr/local/，如果需要指定安装目录，请使用 ./configure --prefix=/your/install/path/
- 需要通过--with-apr指定apr安装目录，如果apr安装到系统目录下需要指定--with-apr=/usr/local/apr/


### SDK安装

```shell
git clone 'https://github.com/aliyun/alibabacloud-httpdns-c-sdk.git'
cd alibabacloud-httpdns-c-sdk
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release   ../
make httpdns_unite_test
sudo make install
sudo ldconfig
```

### SDK使用

安装SDK之后，可以通过静态库或者静态库的方式使用SDK的API(位于文件hdns_api.h)，具体使用方式可以参考代码examples的示例。

注意：
 - 可以通过打开主工程CMakeLists.txt中的ADD_LOG_USE_COLOR开关来实现按日志等级彩色打印内容
 - 可以通过设置主工程CMakeLists.txt中的HTTPDNS_REGION选择HTTPDNS服务集群

## License

- MIT

## 联系我们

- [阿里云HTTPDNS官方文档中心](https://www.aliyun.com/product/httpdns#Docs)
- 阿里云官方技术支持：[提交工单](https://workorder.console.aliyun.com/#/ticket/createIndex)
- 阿里云EMAS开发交流钉钉群：35248489

## 感谢
本项目的内存管理和HTTP层封装从[阿里云OSS C SDK](https://github.com/aliyun/aliyun-oss-c-sdk)中受到了很大的启发，特此感谢。