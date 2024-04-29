//
// Created by cagaoshuai on 2024/4/29.
//

#ifndef HDNS_C_SDK_HDNS_PLATFORM_H
#define HDNS_C_SDK_HDNS_PLATFORM_H

#include "hdns_define.h"

HDNS_CPP_START

#if defined(__APPLE__) || defined(__linux__)

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <unistd.h>

#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
#endif
HDNS_CPP_START

#endif
