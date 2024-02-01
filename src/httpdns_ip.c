//
// Created by caogaoshuai on 2024/1/22.
//

#include "httpdns_ip.h"
#include <string.h>
#include <stdio.h>
#include "sds.h"
#include "httpdns_memory.h"


sds httpdns_ip_to_string(const httpdns_ip_t *httpdns_ip) {
    if (NULL == httpdns_ip) {
        return sdsnew("httpdns_ip_t()");
    }
    char buffer[64];
    sprintf(buffer, "httpdns_ip_t(ip=%s, rt=%d)", httpdns_ip->ip, httpdns_ip->rt);
    return sdsnew(buffer);
}

void httpdns_ip_free(httpdns_ip_t *ip) {
    if (NULL == ip) {
        return;
    }
    if (NULL != ip->ip) {
        sdsfree(ip->ip);
    }
    free(ip);
}

httpdns_ip_t *httpdns_ip_new(const char *ip) {
    if (NULL == ip) {
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(http_ip, httpdns_ip_t);
    http_ip->ip = sdsnew(ip);
    http_ip->rt = DEFAULT_IP_RT;
    return http_ip;
}

httpdns_ip_t *httpdns_ip_clone(const httpdns_ip_t *origin_ip) {
    if (NULL == origin_ip) {
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(new_ip, httpdns_ip_t);
    if (NULL != origin_ip->ip) {
        new_ip->ip = sdsnew(origin_ip->ip);
    }
    new_ip->rt = origin_ip->rt;
    return new_ip;
}

int32_t httpdns_ip_cmp(const httpdns_ip_t *ip1, const httpdns_ip_t *ip2) {
    if (NULL == ip1 && NULL == ip2) {
        return 0;
    }
    if (NULL == ip1 && NULL != ip2) {
        return -1;
    }
    if (NULL != ip1 && NULL == ip2) {
        return 1;
    }
    return ip1->rt - ip2->rt;
}


bool httpdns_ip_search(const httpdns_ip_t *http_ip, const char *ip) {
    if (NULL != http_ip && NULL != ip) {
        return strcmp(http_ip->ip, ip) == 0;
    }
    return false;
}