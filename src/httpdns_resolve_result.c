//
// Created by caogaoshuai on 2024/1/29.
//
#include "httpdns_resolve_result.h"
#include "httpdns_sds.h"
#include "httpdns_memory.h"
#include "log.h"
#include "httpdns_list.h"
#include "httpdns_ip.h"
#include "httpdns_time.h"

httpdns_resolve_result_t *httpdns_resolve_result_new() {
    HTTPDNS_NEW_OBJECT_IN_HEAP(result, httpdns_resolve_result_t);
    httpdns_list_init(&result->ips);
    httpdns_list_init(&result->ipsv6);
    return result;
}


httpdns_resolve_result_t *httpdns_resolve_result_clone(const httpdns_resolve_result_t *origin_result) {
    if (NULL == origin_result) {
        log_info("clone resolve result failed, origin resolve result is NULL");
        return NULL;
    }
    httpdns_resolve_result_t *new_result = httpdns_resolve_result_new();
    memset(new_result, 0, sizeof(httpdns_resolve_result_t));
    if (NULL != origin_result->host) {
        new_result->host = httpdns_sds_new(origin_result->host);
    }
    if (NULL != origin_result->client_ip) {
        new_result->client_ip = httpdns_sds_new(origin_result->client_ip);
    }
    if (NULL != origin_result->extra) {
        new_result->extra = httpdns_sds_new(origin_result->extra);
    }
    if (NULL != origin_result->cache_key) {
        new_result->cache_key = httpdns_sds_new(origin_result->cache_key);
    }
    new_result->ttl = origin_result->ttl;
    new_result->origin_ttl = origin_result->origin_ttl;
    new_result->query_ts = origin_result->query_ts;
    new_result->hit_cache = origin_result->hit_cache;
    httpdns_list_dup(&new_result->ips, &origin_result->ips, to_httpdns_data_clone_func(httpdns_ip_clone));
    httpdns_list_dup(&new_result->ipsv6, &origin_result->ipsv6, to_httpdns_data_clone_func(httpdns_ip_clone));
    return new_result;
}


httpdns_sds_t httpdns_resolve_result_to_string(const httpdns_resolve_result_t *result) {
    if (NULL == result) {
        return httpdns_sds_new("httpdns_resolve_result_t()");
    }


    httpdns_sds_t dst_str = httpdns_sds_new("httpdns_resolve_result_t(");
    httpdns_sds_cat_easily(dst_str, "host=");
    httpdns_sds_cat_easily(dst_str, result->host);
    httpdns_sds_cat_easily(dst_str, ",client_ip=");
    httpdns_sds_cat_easily(dst_str, result->client_ip);
    httpdns_sds_cat_easily(dst_str, ",extra=");
    httpdns_sds_cat_easily(dst_str, result->extra);
    httpdns_sds_cat_easily(dst_str, ",origin_ttl=");
    httpdns_sds_cat_int(dst_str, result->origin_ttl);
    httpdns_sds_cat_easily(dst_str, ",ttl=");
    httpdns_sds_cat_int(dst_str, result->ttl);
    httpdns_sds_cat_easily(dst_str, ",cache_key=");
    httpdns_sds_cat_easily(dst_str, result->cache_key);
    httpdns_sds_cat_easily(dst_str, ",hit_cache=");
    httpdns_sds_cat_int(dst_str, result->hit_cache);
    httpdns_sds_cat_easily(dst_str, ",query_ts=");
    httpdns_sds_t query_ts = httpdns_time_to_string(result->query_ts);
    httpdns_sds_cat_easily(dst_str, query_ts);
    httpdns_sds_free(query_ts);
    httpdns_sds_cat_easily(dst_str, ",ips=");

    httpdns_sds_t list = httpdns_list_to_string(&result->ips, to_httpdns_data_to_string_func(httpdns_ip_to_string));
    httpdns_sds_cat_easily(dst_str, list);
    httpdns_sds_free(list);

    httpdns_sds_cat_easily(dst_str, ",ipsv6=");
    list = httpdns_list_to_string(&result->ipsv6, to_httpdns_data_to_string_func(httpdns_ip_to_string));
    httpdns_sds_cat_easily(dst_str, list);
    httpdns_sds_free(list);
    httpdns_sds_cat_easily(dst_str, ")");
    return dst_str;
}


void httpdns_resolve_result_free(httpdns_resolve_result_t *result) {
    if (NULL == result) {
        return;
    }
    if (NULL != result->host) {
        httpdns_sds_free(result->host);
    }
    if (NULL != result->client_ip) {
        httpdns_sds_free(result->client_ip);
    }
    if (NULL != result->extra) {
        httpdns_sds_free(result->extra);
    }
    if (NULL != result->cache_key) {
        httpdns_sds_free(result->cache_key);
    }
    httpdns_list_free(&result->ips, to_httpdns_data_free_func(httpdns_ip_free));
    httpdns_list_free(&result->ipsv6, to_httpdns_data_free_func(httpdns_ip_free));
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

void httpdns_resolve_results_merge(httpdns_list_head_t *raw_results, httpdns_list_head_t *merged_results) {
    if (NULL == raw_results || NULL == merged_results) {
        log_info("results merge failed, raw results or merged results is NULL");
    }
    httpdns_list_sort(raw_results, to_httpdns_data_cmp_func(httpdns_resolve_result_cmp));
    httpdns_resolve_result_t *target_result = NULL;
    httpdns_list_for_each_entry(curor, raw_results) {
        httpdns_resolve_result_t *cur_result = curor->data;
        if (NULL == target_result || strcmp(target_result->host, cur_result->host) != 0) {
            target_result = httpdns_resolve_result_clone(cur_result);
            httpdns_list_add(merged_results, target_result, NULL);
        } else {
            if (httpdns_list_is_not_empty(&cur_result->ips) && httpdns_list_is_empty(&target_result->ips)) {
                httpdns_list_dup(&target_result->ips, &cur_result->ips, to_httpdns_data_clone_func(httpdns_ip_clone));
            }
            if (httpdns_list_is_not_empty(&cur_result->ipsv6) && httpdns_list_is_empty(&target_result->ipsv6)) {
                httpdns_list_dup(&target_result->ipsv6, &cur_result->ipsv6, to_httpdns_data_clone_func(httpdns_ip_clone));
            }
            if (cur_result->origin_ttl > target_result->origin_ttl) {
                target_result->origin_ttl = cur_result->origin_ttl;
            }
            if (cur_result->ttl > target_result->ttl) {
                target_result->ttl = cur_result->ttl;
            }
        }
    }
}