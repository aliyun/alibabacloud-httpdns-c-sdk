//
// Created by cagaoshuai on 2024/1/9.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LOGGER_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LOGGER_H


#if defined(__ANDROID__)
#include <android/log.h>
#define  LOG_TAG    "HTTPDNS"
#ifndef HTTPDNS_INFO
#define HTTPDNS_INFO(...) do { \
                                __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__); \
                               } while(0)
#endif
#elif defined(__APPLE__)

#include <stdio.h>

#ifndef HTTPDNS_INFO
#define HTTPDNS_INFO(msg) printf("INFO: %s\n", msg)
#endif

#else
#include <stdio.h>

#ifndef HTTPDNS_INFO
#define HTTPDNS_INFO(msg) printf("INFO: %s\n", msg)
#endif
#endif


#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LOGGER_H
