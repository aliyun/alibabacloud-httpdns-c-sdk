//
// Created by cagaoshuai on 2024/1/18.
//
#include "httpdns_result.h"
#include "sds.h"
#include "httpdns_list.h"
#include "httpdns_memory.h"
#include "httpdns_time.h"
#include "response_parser.h"

void print_httpdns_ip(httpdns_ip_t *httpdns_ip) {
    if (NULL != httpdns_ip) {
        printf("httpdns_ip:");
        printf("{ip:%s, rt:%d}", httpdns_ip->ip, httpdns_ip->rt);
    }
}






void destroy_httpdns_ip(httpdns_ip_t *httpdns_ip) {
    if (NULL == httpdns_ip) {
        return;
    }
    if (IS_NOT_BLANK_STRING(httpdns_ip->ip)) {
        sdsfree(httpdns_ip->ip);
    }
    free(httpdns_ip);
}

httpdns_ip_t *create_httpdns_ip(char *ip) {
    if (NULL == ip) {
        return NULL;
    }
    httpdns_ip_t *http_ip = (httpdns_ip_t *) malloc(sizeof(httpdns_ip_t));
    memset(http_ip, 0, sizeof(httpdns_ip_t));
    http_ip->ip = sdsnew(ip);
    http_ip->rt = DEFAULT_IP_RT;
    return http_ip;
}

httpdns_ip_t *clone_httpdns_ip(httpdns_ip_t *origin_ip) {
    if (NULL == origin_ip) {
        return NULL;
    }
    httpdns_ip_t *ip_copy = (httpdns_ip_t *) malloc(sizeof(httpdns_ip_t));
    memset(ip_copy, 0, sizeof(httpdns_ip_t));
    ip_copy->ip = sdsnew(origin_ip->ip);
    ip_copy->rt = origin_ip->rt;
    return ip_copy;
}

void destroy_httpdns_resolve_result(httpdns_resolve_result_t *result) {
    if (NULL == result) {
        return;
    }
    if (IS_NOT_BLANK_STRING(result->host)) {
        sdsfree(result->host);
    }
    if (IS_NOT_BLANK_STRING(result->client_ip)) {
        sdsfree(result->client_ip);
    }
    if (IS_NOT_BLANK_STRING(result->extra)) {
        sdsfree(result->extra);
    }
    if (IS_NOT_BLANK_STRING(result->cache_key)) {
        sdsfree(result->cache_key);
    }
    httpdns_list_free(&result->ips, DATA_FREE_FUNC(destroy_httpdns_ip));
    httpdns_list_free(&result->ipsv6, DATA_FREE_FUNC(destroy_httpdns_ip));
    free(result);
}


//char *host;
//char *client_ip;
//char *extra;
//struct list_head ips;
//struct list_head ipsv6;
//int origin_ttl;
//int ttl;
//struct timespec query_ts;
//char *cache_key;
//bool hit_cache;

void print_httpdns_resolve_result(httpdns_resolve_result_t *result) {
    if(NULL == result){
        return;
    }
    char query_ts[32];
    httpdns_time_to_string(result->query_ts, query_ts, 32);
    printf("{\n");
    printf("host=%s, client_ip=%s, extra=%s, origin_ttl=%d, ttl=%d, cache_key=%s, hit_cache=%d, query_ts=%s",
    result->host, result->client_ip, result->extra, result->origin_ttl, result->ttl, result->cache_key, result->hit_cache, query_ts);
    printf(",ips=");
    httpdns_list_print(&result->ips, DATA_PRINT_FUNC(print_httpdns_ip));
    printf(",ipsv6=");
    httpdns_list_print(&result->ipsv6, DATA_PRINT_FUNC(print_httpdns_ip));
    printf("\n}");
}


httpdns_resolve_result_t *clone_httpdns_resolve_result(httpdns_resolve_result_t *origin_result) {
    if (NULL == origin_result) {
        return NULL;
    }
    httpdns_resolve_result_t *result_copy = (httpdns_resolve_result_t *) malloc(sizeof(httpdns_resolve_result_t));
    memset(result_copy, 0, sizeof(httpdns_resolve_result_t));
    if (NULL != origin_result->host) {
        result_copy->host = sdsnew(origin_result->host);
    }
    if (NULL != origin_result->client_ip) {
        result_copy->client_ip = sdsnew(origin_result->client_ip);
    }
    if (NULL != origin_result->extra) {
        result_copy->extra = sdsnew(origin_result->extra);
    }
    if (NULL != origin_result->cache_key) {
        result_copy->cache_key = sdsnew(origin_result->cache_key);
    }
    result_copy->ttl = origin_result->ttl;
    result_copy->origin_ttl = origin_result->origin_ttl;
    result_copy->query_ts = origin_result->query_ts;
    result_copy->hit_cache = origin_result->hit_cache;
    httpdns_list_init(&result_copy->ips);
    httpdns_list_init(&result_copy->ipsv6);
    httpdns_list_dup(&result_copy->ips, &origin_result->ips, DATA_CLONE_FUNC(clone_httpdns_ip));
    httpdns_list_dup(&result_copy->ipsv6, &origin_result->ipsv6, DATA_CLONE_FUNC(clone_httpdns_ip));
    return result_copy;
}












