//
// Created by cagaoshuai on 2024/1/22.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_IP_H
#define HTTPDNS_C_SDK_HTTPDNS_IP_H

#include <stdint.h>
#include<stdlib.h>
#include<stdbool.h>
#include "sds.h"


#define DEFAULT_IP_RT         0

typedef struct {
    char *ip;
    int32_t rt;
} httpdns_ip_t;


httpdns_ip_t *httpdns_ip_new(const char *ip);

sds httpdns_ip_to_string(const httpdns_ip_t *httpdns_ip);

httpdns_ip_t *httpdns_ip_clone(const httpdns_ip_t *ip);

void httpdns_ip_free(httpdns_ip_t *ip);

int32_t httpdns_ip_cmp(const httpdns_ip_t *ip1, const httpdns_ip_t *ip2);

bool httpdns_ip_search(const httpdns_ip_t *http_ip, const char *ip);

#endif //HTTPDNS_C_SDK_HTTPDNS_IP_H
