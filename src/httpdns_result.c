//
// Created by cagaoshuai on 2024/1/18.
//
#include "httpdns_result.h"
#include "sds.h"
#include "httpdns_list.h"
#include "httpdns_memory.h"


void print_httpdns_ip(httpdns_ip_t *httpdns_ip) {
    if (NULL != httpdns_ip) {
        printf("httpdns_ip:");
        printf("{ip:%s, rt:%d}", httpdns_ip->ip, httpdns_ip->rt);
    }
}

void print_httpdns_raw_schedule_result(httpdns_raw_schedule_result_t *result) {
    if (NULL != result) {
        printf("\n{");
        printf("\nraw_schedule_result:");
        printf("\nservice_ip:\n");
        httpdns_list_print(&result->service_ip, DATA_PRINT_FUNC(print_httpdns_ip));
        printf("service_ipv6:\n");
        httpdns_list_print(&result->service_ipv6, DATA_PRINT_FUNC(print_httpdns_ip));
        printf("}\n");
    }
}

void print_httpdns_raw_single_resolve_result(httpdns_raw_single_resolve_result_t *result) {
    if (NULL != result) {
        printf("\nhost:%s", result->host);
        printf("\nips:\n");
        httpdns_list_print(&result->ips, DATA_FREE_FUNC(print_httpdns_ip));
        printf("ipsv6:\n");
        httpdns_list_print(&result->ipsv6, DATA_FREE_FUNC(print_httpdns_ip));
        printf("ttl:%d", result->ttl);
        printf("\norigin_ttl:%d", result->origin_ttl);
        printf("\nextra:%s", result->extra);
        printf("\nclient_ip:%s", result->client_ip);
        printf("\ntype:%d", result->type);
    }
}

void print_httpdns_raw_multi_resolve_result(httpdns_raw_multi_resolve_result_t *result) {
    if (NULL != result) {
        printf("\nraw_multi_resolve_result:");
        httpdns_list_print(&result->dns, DATA_PRINT_FUNC(print_httpdns_raw_single_resolve_result));
    }
}

void destroy_httpdns_ip(httpdns_ip_t *httpdns_ip) {
    if (NULL == httpdns_ip) {
        return;
    }
    if (IS_NOT_BLANK_SDS(httpdns_ip->ip)) {
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
    httpdns_list_init(&result_copy->ipv6s);
    httpdns_list_dup(&result_copy->ips, &origin_result->ips, DATA_CLONE_FUNC(clone_httpdns_ip));
    httpdns_list_dup(&result_copy->ipv6s, &origin_result->ipv6s, DATA_CLONE_FUNC(clone_httpdns_ip));
    return result_copy;
}

httpdns_raw_schedule_result_t *create_httpdns_raw_schedule_result() {
    HTTPDNS_NEW_OBJECT_IN_HEAP(schedule_result, httpdns_raw_schedule_result_t);
    httpdns_list_init(&schedule_result->service_ip);
    httpdns_list_init(&schedule_result->service_ipv6);
    return schedule_result;
}


void destroy_httpdns_raw_schedule_result(httpdns_raw_schedule_result_t *result) {
    if (NULL != result) {
        httpdns_list_free(&result->service_ip, DATA_FREE_FUNC(destroy_httpdns_ip));
        httpdns_list_free(&result->service_ipv6, DATA_FREE_FUNC(destroy_httpdns_ip));
        free(result);
    }
}

httpdns_raw_single_resolve_result_t *create_httpdns_raw_single_resolve_result() {
    HTTPDNS_NEW_OBJECT_IN_HEAP(single_resolve_result, httpdns_raw_single_resolve_result_t);
    httpdns_list_init(&single_resolve_result->ips);
    httpdns_list_init(&single_resolve_result->ipsv6);
    return single_resolve_result;
}

void destroy_httpdns_raw_single_resolve_result(httpdns_raw_single_resolve_result_t *result) {
    if (NULL != result) {
        if (NULL != result->host) {
            sdsfree(result->host);
        }
        if (NULL != result->extra) {
            sdsfree(result->extra);
        }
        if (NULL != result->client_ip) {
            sdsfree(result->client_ip);
        }
        httpdns_list_free(&result->ips, DATA_FREE_FUNC(destroy_httpdns_ip));
        httpdns_list_free(&result->ipsv6, DATA_FREE_FUNC(destroy_httpdns_ip));
        free(result);
    }
}

httpdns_raw_multi_resolve_result_t *create_httpdns_raw_multi_resolve_result() {
    HTTPDNS_NEW_OBJECT_IN_HEAP(multi_resolve_result, httpdns_raw_multi_resolve_result_t);
    httpdns_list_init(&multi_resolve_result->dns);
    return multi_resolve_result;
}

void destroy_httpdns_raw_multi_resolve_result(httpdns_raw_multi_resolve_result_t *result) {
    if (NULL != result) {
        httpdns_list_free(&result->dns, DATA_FREE_FUNC(destroy_httpdns_raw_single_resolve_result));
        free(result);
    }
}
