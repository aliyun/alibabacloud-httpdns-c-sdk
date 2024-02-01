//
// Created by cagaoshuai on 2024/1/29.
//
#include "httpdns_resolve_result.h"
#include "httpdns_string.h"
#include "httpdns_memory.h"
#include "log.h"
#include "httpdns_list.h"
#include "httpdns_ip.h"
#include "httpdns_time.h"

httpdns_resolve_result_t *httpdns_resolve_result_clone(const httpdns_resolve_result_t *origin_result) {
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


sds httpdns_resolve_result_to_string(const httpdns_resolve_result_t *result) {
    if (NULL == result) {
        return sdsnew("httpdns_resolve_result_t()");
    }


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
    sds query_ts = httpdns_time_to_string(result->query_ts);
    SDS_CAT(dst_str, query_ts);
    sdsfree(query_ts);
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


void httpdns_resolve_result_set_cache_key(httpdns_resolve_result_t *result, const char *cache_key) {
    HTTPDNS_SET_STRING_FIELD(result, cache_key, cache_key);
}

void httpdns_resolve_result_set_hit_cache(httpdns_resolve_result_t *result, bool hit_cache) {
    if (NULL != result) {
        result->hit_cache = hit_cache;
    }
}

int32_t httpdns_resolve_result_cmp(const httpdns_resolve_result_t *result1, const httpdns_resolve_result_t *result2) {
    if (NULL == result1 && NULL == result2) {
        return 0;
    }
    if (NULL == result1 && NULL != result2) {
        return -1;
    }
    if (NULL != result1 && NULL == result2) {
        return 1;
    }
    return strcmp(result1->host, result2->host);
}

void httpdns_resolve_results_merge(struct list_head *raw_results, struct list_head *merged_results) {
    if (NULL == raw_results || NULL == merged_results) {
        log_info("results merge failed, raw results or merged results is NULL");
    }
    httpdns_list_sort(raw_results, DATA_CMP_FUNC(httpdns_resolve_result_cmp));
    httpdns_resolve_result_t *target_result = NULL;
    httpdns_list_for_each_entry(curor, raw_results) {
        httpdns_resolve_result_t *cur_result = curor->data;
        if (NULL == target_result || strcmp(target_result->host, cur_result->host) != 0) {
            target_result = httpdns_resolve_result_clone(cur_result);
            httpdns_list_add(merged_results, target_result, NULL);
        } else {
            if(IS_NOT_EMPTY_LIST(&cur_result->ips) && IS_EMPTY_LIST(&target_result->ips)) {
               httpdns_list_dup(&target_result->ips, &cur_result->ips, DATA_CLONE_FUNC(httpdns_ip_clone));
            }
            if(IS_NOT_EMPTY_LIST(&cur_result->ipsv6) && IS_EMPTY_LIST(&target_result->ipsv6)) {
                httpdns_list_dup(&target_result->ipsv6, &cur_result->ipsv6, DATA_CLONE_FUNC(httpdns_ip_clone));
            }
            if(cur_result->origin_ttl > target_result->origin_ttl) {
                target_result->origin_ttl = cur_result->origin_ttl;
            }
            if(cur_result->ttl > target_result->ttl) {
                target_result->ttl = cur_result->ttl;
            }
        }
    }
}