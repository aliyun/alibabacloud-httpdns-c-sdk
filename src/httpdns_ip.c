//
// Created by caogaoshuai on 2024/1/22.
//

#include "httpdns_ip.h"
#include <string.h>
#include <stdio.h>
#include "httpdns_sds.h"
#include "httpdns_memory.h"


httpdns_sds_t httpdns_ip_to_string(const httpdns_ip_t *httpdns_ip) {
    if (NULL == httpdns_ip) {
        return httpdns_sds_new("httpdns_ip_t()");
    }
    char buffer[64];
    sprintf(buffer, "httpdns_ip_t(ip=%s, rt=%d)", httpdns_ip->ip, httpdns_ip->rt);
    return httpdns_sds_new(buffer);
}

void httpdns_ip_free(httpdns_ip_t *ip) {
    if (NULL == ip) {
        return;
    }
    if (NULL != ip->ip) {
        httpdns_sds_free(ip->ip);
    }
    free(ip);
}

httpdns_ip_t *httpdns_ip_new(const char *ip) {
    if (NULL == ip) {
        return NULL;
    }
    httpdns_new_object_in_heap(http_ip, httpdns_ip_t);
    http_ip->ip = httpdns_sds_new(ip);
    http_ip->rt = HTTPDNS_DEFAULT_IP_RT;
    return http_ip;
}

httpdns_ip_t *httpdns_ip_clone(const httpdns_ip_t *origin_ip) {
    if (NULL == origin_ip) {
        return NULL;
    }
    httpdns_new_object_in_heap(new_ip, httpdns_ip_t);
    if (NULL != origin_ip->ip) {
        new_ip->ip = httpdns_sds_new(origin_ip->ip);
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