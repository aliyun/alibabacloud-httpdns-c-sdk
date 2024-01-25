//
// Created by cagaoshuai on 2024/1/11.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_NET_STACK_DETECTOR_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_NET_STACK_DETECTOR_H

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
#include "sds.h"

#define IPV4_PROBE_ADDR  "8.8.8.8"
#define IPV6_PROBE_ADDR  "2000::"
#define PROBE_DOMAIN     "www.taobao.com"
#define PROBE_PORT        0xFFFF

#define IP_STACK_UNKNOWN 0x0000
#define IPV4_ONLY        0x0001
#define IPV6_ONLY        0x0002
#define IP_DUAL_STACK    0x0003

#define ADD_IPV4_NET_TYPE(net_stack_type) \
net_stack_type = net_stack_type | (1 << 0)

#define ADD_IPV6_NET_TYPE(net_stack_type) \
net_stack_type = net_stack_type | (1 << 1)

#define HAVE_IPV4_NET_TYPE(net_stack_type) \
net_stack_type & (1<<0)

#define HAVE_IPV6_NET_TYPE(net_stack_type) \
net_stack_type & (1<<1)

typedef u_int32_t net_stack_type_t;

typedef struct {
    net_stack_type_t net_stack_type_cache;
    char *probe_domain;
    bool using_cache;
} httpdns_net_stack_detector_t;


httpdns_net_stack_detector_t *httpdns_net_stack_detector_create();

void httpdns_net_stack_detector_destroy(httpdns_net_stack_detector_t *detector);

void httpdns_net_stack_detector_update_cache(httpdns_net_stack_detector_t *detector);

void httpdns_net_stack_detector_set_using_cache(httpdns_net_stack_detector_t *detector, bool using_cache);

void httpdns_net_stack_detector_set_probe_domain(httpdns_net_stack_detector_t *detector, const char *probe_domain);

net_stack_type_t httpdns_net_stack_type_get(httpdns_net_stack_detector_t *detector);


#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_NET_STACK_DETECTOR_H
