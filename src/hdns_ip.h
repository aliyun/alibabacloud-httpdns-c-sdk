//
// Created by caogaoshuai on 2024/1/22.
//

#ifndef HDNS_C_SDK_HDNS_IP_H
#define HDNS_C_SDK_HDNS_IP_H

#include "hdns_define.h"

HDNS_CPP_START
#define HDNS_DEFAULT_IP_RT         0

typedef struct {
    char *ip;
    //单位：微秒
    int32_t rt;
} hdns_ip_t;

hdns_ip_t *hdns_ip_create(hdns_pool_t *pool, const char *ip);

int32_t hdns_ip_cmp(const hdns_ip_t *ip1, const hdns_ip_t *ip2);

bool hdns_ip_search(const hdns_ip_t *http_ip, const char *ip);

HDNS_CPP_END

#endif
