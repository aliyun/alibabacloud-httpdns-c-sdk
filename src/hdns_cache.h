//
// Created by caogaoshuai on 2024/1/10.
//

#ifndef HDNS_C_SDK_HDNS_CACHE_H
#define HDNS_C_SDK_HDNS_CACHE_H

#include "hdns_resolver.h"
#include "hdns_define.h"

HDNS_CPP_START

typedef struct {
    hdns_pool_t *pool;
    hdns_hash_t *v4_table;
    hdns_hash_t *v6_table;
    apr_thread_mutex_t *lock;
} hdns_cache_t;

typedef hdns_resv_resp_t hdns_cache_entry_t;

static APR_INLINE bool hdns_cache_entry_is_expired(hdns_cache_entry_t *entry) {
    int64_t ttl = entry->origin_ttl > 0 ? entry->origin_ttl : entry->ttl;
    return entry->query_time + ttl * APR_USEC_PER_SEC <= apr_time_now();
}

hdns_cache_t *hdns_cache_table_create();

int32_t hdns_cache_table_add(hdns_cache_t *cache, const hdns_cache_entry_t *entry);

int32_t hdns_cache_table_delete(hdns_cache_t *cache, const char *key, hdns_rr_type_t type);

hdns_cache_entry_t *hdns_cache_table_get(hdns_cache_t *cache, const char *key, hdns_rr_type_t type);

void hdns_cache_table_clean(hdns_cache_t *cache_table);

void hdns_cache_table_cleanup(hdns_cache_t *cache_table);

hdns_list_head_t *hdns_cache_get_keys(hdns_cache_t *cache, hdns_rr_type_t type);

HDNS_CPP_END

#endif
