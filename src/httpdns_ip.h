//
// Created by caogaoshuai on 2024/1/22.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_IP_H
#define HTTPDNS_C_SDK_HTTPDNS_IP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

#include "httpdns_sds.h"


#define HTTPDNS_DEFAULT_IP_RT         0

typedef struct {
    char *ip;
    int32_t rt;
} httpdns_ip_t;

/**
 * must free using httpdns_ip_free
 */
httpdns_ip_t *httpdns_ip_new(const char *ip);

/**
 * must free using httpdns_sds_free
 */
httpdns_sds_t httpdns_ip_to_string(const httpdns_ip_t *httpdns_ip);
/**
 * must free using httpdns_ip_free
 */
httpdns_ip_t *httpdns_ip_clone(const httpdns_ip_t *ip);

void httpdns_ip_free(httpdns_ip_t *ip);

int32_t httpdns_ip_cmp(const httpdns_ip_t *ip1, const httpdns_ip_t *ip2);

bool httpdns_ip_search(const httpdns_ip_t *http_ip, const char *ip);

#ifdef __cplusplus
}
#endif

#endif //HTTPDNS_C_SDK_HTTPDNS_IP_H
