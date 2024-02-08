阿里云EMAS提供HTTPDNS C SDK，以帮助开发者降低在嵌入式、Linux、Windows、Mac等场景下接入HTTPDNS的门槛。

# SDK 特点
* 高可用
  * 内置多组启动IP
  * 解析失败自动重试
* 性能
  * 支持异步解析
  * 支持http2协议
  * 支持本地缓存
  * 支持批量解析
  * 支持预解析
* 安全
  * 支持https
  * 请求加签防盗刷
* 解析类型
  * 域名A记录或AAAA记录
  * 支持根据网络类型自行解析对应的记录类型
* 易用
  * 屏蔽https ip证书校验等细节
  * 提供示例Example程序

# SDK限制
 目前只适配了Mac M1、Linux平台，暂不支持Windows、Android、IOS。

# 功能优势

* 性能：支持批量解析

# 安装方法

### 下载HTTPDNS C SDK代码

您可以使用如下命令获取代码：

```shell
git clone https://github.com/aliyun/alicloud-httpdns-c-sdk.git
```

### 环境依赖

需要安装openssl
HTTPDNS C SDK使用curl进行网络操作，您需要确认这些库已经安装，并且将它们的头文件目录和库文件目录都加入到了项目中。

#### libcurl下载以及安装

libcurl建议 7.49.0 及以上版本

请从[这里](http://curl.haxx.se/download.html)下载，并参考[libcurl 安装指南](http://curl.haxx.se/docs/install.html)
安装。典型的安装方式如下：

```shell
    ./configure
    make
    make install
```

* 注意：

- 执行./configure时默认是配置安装目录为/usr/local/，如果需要指定安装目录，请使用 ./configure --prefix=/your/install/path/

#### check测试框架

```shell

git clone https://github.com/libcheck/check.git
 
cd check/
mkdir build
cd build/
cmake ..
make
CTEST_OUTPUT_ON_FAILURE=1 make test

```

#### HTTPDNS C SDK的安装

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

## 使用

一个应用可创建多个resolver，每个resolver可单独配置账户、协议、验签等信息。
