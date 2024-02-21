//
// Created by caogaoshuai on 2024/1/11.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_NET_STACK_DETECTOR_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_NET_STACK_DETECTOR_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @description Detect the type of network stack

 * @refer https://android.googlesource.com/platform/bionic/+/085543106/libc/results/net/getaddrinfo.c
 *
}
 */
#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <netdb.h>
#include "httpdns_error_type.h"
#include "httpdns_sds.h"

#define HTTPDNS_IPV4_PROBE_ADDR  "8.8.8.8"
#define HTTPDNS_IPV6_PROBE_ADDR  "2000::"
#define HTTPDNS_PROBE_DOMAIN     "www.taobao.com"
#define HTTPDNS_PROBE_PORT        0xFFFF

#define HTTPDNS_IP_STACK_UNKNOWN 0x0000
#define HTTPDNS_IPV4_ONLY        0x0001
#define HTTPDNS_IPV6_ONLY        0x0002
#define HTTPDNS_IP_DUAL_STACK    0x0003

#define httpdns_add_ipv4_net_type(net_stack_type) \
    net_stack_type = net_stack_type | (1 << 0)

#define httpdns_add_ipv6_net_type(net_stack_type) \
    net_stack_type = net_stack_type | (1 << 1)

#define httpdns_have_ipv4_net_type(net_stack_type) \
    net_stack_type & (1<<0)

#define httpdns_have_ipv6_net_type(net_stack_type) \
    net_stack_type & (1<<1)

typedef u_int32_t net_stack_type_t;

typedef struct {
    volatile net_stack_type_t net_stack_type_cache;
    char *probe_domain;
    volatile bool using_cache;
} httpdns_net_stack_detector_t;

/**
 * must free using httpdns_net_stack_detector_free
 */
httpdns_net_stack_detector_t *httpdns_net_stack_detector_new();

void httpdns_net_stack_detector_free(httpdns_net_stack_detector_t *detector);

void httpdns_net_stack_detector_update_cache(httpdns_net_stack_detector_t *detector);

void httpdns_net_stack_detector_set_using_cache(httpdns_net_stack_detector_t *detector, bool using_cache);

void httpdns_net_stack_detector_set_probe_domain(httpdns_net_stack_detector_t *detector, const char *probe_domain);

net_stack_type_t httpdns_net_stack_type_get(httpdns_net_stack_detector_t *detector);

#ifdef __cplusplus
}
#endif

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_NET_STACK_DETECTOR_H
