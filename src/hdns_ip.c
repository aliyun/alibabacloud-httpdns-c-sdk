//
// Created by caogaoshuai on 2024/1/22.
//

#include "hdns_ip.h"


hdns_ip_t *hdns_ip_create(hdns_pool_t *pool, const char *ip) {
    if (NULL == ip) {
        return NULL;
    }
    hdns_ip_t *hdns_ip = hdns_palloc(pool, sizeof(hdns_ip_t));
    hdns_ip->ip = apr_pstrdup(pool, ip);
    hdns_ip->rt = HDNS_DEFAULT_IP_RT;
    return hdns_ip;
}


int32_t hdns_ip_cmp(const hdns_ip_t *ip1, const hdns_ip_t *ip2) {
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


bool hdns_ip_search(const hdns_ip_t *http_ip, const char *ip) {
    if (NULL != http_ip && NULL != ip) {
        return strcmp(http_ip->ip, ip) == 0;
    }
    return false;
}