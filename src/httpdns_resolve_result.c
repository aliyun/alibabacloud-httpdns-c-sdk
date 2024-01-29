//
// Created by cagaoshuai on 2024/1/29.
//
#include "httpdns_resolve_result.h"
#include "httpdns_memory.h"
#include "log.h"
#include "httpdns_list.h"
#include "httpdns_ip.h"
#include "httpdns_time.h"

httpdns_resolve_result_t *httpdns_resolve_result_clone(httpdns_resolve_result_t *origin_result) {
    if (NULL == origin_result) {
        log_info("clone resolve result failed, origin resolve result is NULL");
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(new_result, httpdns_resolve_result_t);
    memset(new_result, 0, sizeof(httpdns_resolve_result_t));
    if (NULL != origin_result->host) {
        new_result->host = sdsnew(origin_result->host);
    }
    if (NULL != origin_result->client_ip) {
        new_result->client_ip = sdsnew(origin_result->client_ip);
    }
    if (NULL != origin_result->extra) {
        new_result->extra = sdsnew(origin_result->extra);
    }
    if (NULL != origin_result->cache_key) {
        new_result->cache_key = sdsnew(origin_result->cache_key);
    }
    new_result->ttl = origin_result->ttl;
    new_result->origin_ttl = origin_result->origin_ttl;
    new_result->query_ts = origin_result->query_ts;
    new_result->hit_cache = origin_result->hit_cache;
    httpdns_list_dup(&new_result->ips, &origin_result->ips, DATA_CLONE_FUNC(httpdns_ip_clone));
    httpdns_list_dup(&new_result->ipsv6, &origin_result->ipsv6, DATA_CLONE_FUNC(httpdns_ip_clone));
    return new_result;
}


sds httpdns_resolve_result_to_string(httpdns_resolve_result_t *result) {
    if (NULL == result) {
        return sdsnew("httpdns_resolve_result_t()");
    }
    char query_ts[32];
    httpdns_time_to_string(result->query_ts, query_ts, 32);
    sds dst_str = sdsnew("httpdns_resolve_result_t(");
    SDS_CAT(dst_str, "host=");
    SDS_CAT(dst_str, result->host);
    SDS_CAT(dst_str, ",client_ip=");
    SDS_CAT(dst_str, result->client_ip);
    SDS_CAT(dst_str, ",extra=");
    SDS_CAT(dst_str, result->extra);
    SDS_CAT(dst_str, ",origin_ttl=");
    SDS_CAT_INT(dst_str, result->origin_ttl);
    SDS_CAT(dst_str, ",ttl=");
    SDS_CAT_INT(dst_str, result->ttl);
    SDS_CAT(dst_str, ",cache_key=");
    SDS_CAT(dst_str, result->cache_key);
    SDS_CAT(dst_str, ",hit_cache=");
    SDS_CAT_INT(dst_str, result->hit_cache);
    SDS_CAT(dst_str, ",query_ts=");
    SDS_CAT(dst_str, query_ts);
    SDS_CAT(dst_str, ",ips=");

    sds list = httpdns_list_to_string(&result->ips, DATA_TO_STRING_FUNC(httpdns_ip_to_string));
    SDS_CAT(dst_str, list);
    sdsfree(list);

    SDS_CAT(dst_str, ",ipsv6=");
    list = httpdns_list_to_string(&result->ipsv6, DATA_TO_STRING_FUNC(httpdns_ip_to_string));
    SDS_CAT(dst_str, list);
    sdsfree(list);
    SDS_CAT(dst_str, ")");
    return dst_str;
}


void httpdns_resolve_result_free(httpdns_resolve_result_t *result) {
    if (NULL == result) {
        return;
    }
    if (NULL != result->host) {
        sdsfree(result->host);
    }
    if (NULL != result->client_ip) {
        sdsfree(result->client_ip);
    }
    if (NULL != result->extra) {
        sdsfree(result->extra);
    }
    if (NULL != result->cache_key) {
        sdsfree(result->cache_key);
    }
    httpdns_list_free(&result->ips, DATA_FREE_FUNC(httpdns_ip_free));
    httpdns_list_free(&result->ipsv6, DATA_FREE_FUNC(httpdns_ip_free));
    free(result);
}


void httpdns_resolve_result_set_cache_key(httpdns_resolve_result_t *result, char *cache_key) {
    HTTPDNS_SET_STRING_FIELD(result, cache_key, cache_key);
}

void httpdns_resolve_result_set_hit_cache(httpdns_resolve_result_t *result, bool hit_cache) {
    if (NULL != result) {
        result->hit_cache = hit_cache;
    }
}