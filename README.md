# Aliyun HTTPDNS SDK for C

[![GitHub version](https://badge.fury.io/gh/aliyun%2Falibabacloud-httpdns-c-sdk.git.svg)](https://badge.fury.io/gh/aliyun%2Falibabacloud-httpdns-c-sdk.git)
[![Software License](https://img.shields.io/badge/license-MIT-brightgreen.svg)](LICENSE)

## 关于

阿里云[HTTPDNS](https://www.aliyun.com/product/httpdns)
是面向多端应用（移动端APP、PC客户端应用、嵌入式应用等）具有防劫持、精准调度、实时解析生效等特性的域名解析服务。阿里云EMAS团队提供了C
SDK，
以降低开发者通过C\C++语言接入[HTTPDNS](https://www.aliyun.com/product/httpdns)的门槛，用户可以通过调用相关API方便地使用HTTPDNS进行域名解析。

## SDK限制

目前仅适配了Linux、macOS、Windows，暂不支持Android、IOS、RTOS等平台。

## 版本

- 当前版本：2.1.0

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
sudo yum install -y git cmake  gcc  gcc-c++
```

- OpenSUSE:

```shell
sudo zypper refresh
sudo zypper install -y git cmake  gcc  gcc-c++
```

- macOS

```shell
export HOMEBREW_NO_AUTO_UPDATE=1
brew install git gcc cmake
```

- Windows

[下载安装Git](https://git-scm.com/downloads)、[下载安装Visual Studio](https://visualstudio.microsoft.com/zh-hans/vs/) (
工作负载选择“使用C++的桌面开发”)

### 依赖库安装

SDK使用curl库(版本7.33.0及以上)进行网络操作，使用openssl库(版本1.1.0及以上)
进行HTTPS的SSL层校验，使用apr/apr-util(版本1.5.2及以上)库解决内存管理以及跨平台问题，使用cjson库解析服务端响应报文，
SDK并没有带上这几个外部库，您需要确认这些库已经安装，并且将它们的头文件目录和库文件目录都加入到了项目中。
本项目支持[VCPKG](https://github.com/microsoft/vcpkg)安装和手动安装两种方式安装这些C/C++库。

#### VCPKG安装

- [安装VCPG](https://github.com/microsoft/vcpkg)
- 安装SDK依赖的库
    - macOS/Linux: ./vcpkg install apr apr-util openssl curl[openssl,http2]  cjson
    - Windows: .\vcpkg.exe install apr apr-util openssl curl[openssl,http2]  cjson

#### 手动安装

- Ubuntu/Debian:

```shell
sudo apt install libssl-dev libcurl4-openssl-dev libapr1-dev libaprutil1-dev
```

- Aliyun/CentOS Stream/Fedora:

```shell
sudo yum install openssl-devel libcurl-devel apr-util apr-devel apr-util-devel
```

- OpenSUSE:

```shell
sudo zypper install libopenssl-devel libcurl-devel libapr1-devel libapr-util1-devel 
```

- macOS

```shell
brew install  curl openssl apr apr-util cjson
```

- Windows(源码安装)

[下载安装cjson](https://github.com/DaveGamble/cJSON)、
[下载安装openssl](http://slproweb.com/products/Win32OpenSSL.html)、
[下载](http://curl.haxx.se/download.html)[安装curl](http://curl.haxx.se/docs/install.html)、
[下载安装apr/apr-util](https://apr.apache.org/download.cgi)


<span style="color:red;">
注意：libcurl必须是依赖OpenSSL完成的SSL层通信，否则可能会导致程序异常，可以通过以下命令行检验当前libcurl是否使用了OpenSSL：</span>

```shell
curl  --version | grep -i -o  OpenSSL
```

### SDK安装

- macOS/Linux:

```shell
git clone https://github.com/aliyun/alibabacloud-httpdns-c-sdk.git
cd alibabacloud-httpdns-c-sdk
mkdir build
cd build
# VCPKG安装库时采用下面命令安装
# cmake -DCMAKE_BUILD_TYPE=Release  -DVCPKG_ROOT=${vcpkg的安装路径}  ../ 
cmake -DCMAKE_BUILD_TYPE=Release ../ 
make httpdns_unite_test
sudo make install
sudo ldconfig
```

- Windows:
    * 下载工程
    * Visual Studio打开Cmake工程
    * 管理配置中配置Cmake命令参数：-DVCPKG_ROOT=${vcpkg的安装路径}

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