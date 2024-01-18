//
// Created by cagaoshuai on 2024/1/18.
//
#include "httpdns_result.h"
#include "sds.h"
#include "httpdns_list.h"

void destroy_httpdns_ip(httpdns_ip_t *httpdns_ip) {
    if (NULL == httpdns_ip) {
        return;
    }
    if (IS_NOT_BLANK_SDS(httpdns_ip->ip)) {
        sdsfree(httpdns_ip->ip);
    }
    free(httpdns_ip);
}

void destroy_httpdns_resolve_result(httpdns_resolve_result_t *result) {
    if (NULL == result) {
        return;
    }
    if (IS_NOT_BLANK_SDS(result->host)) {
        sdsfree(result->host);
    }
    if (IS_NOT_BLANK_SDS(result->client_ip)) {
        sdsfree(result->client_ip);
    }
    if (IS_NOT_BLANK_SDS(result->extra)) {
        sdsfree(result->extra);
    }
    httpdns_list_free(&result->ips, DATA_FREE_FUNC(destroy_httpdns_ip));
    httpdns_list_free(&result->ipv6s, DATA_FREE_FUNC(destroy_httpdns_ip));
    free(result);
}