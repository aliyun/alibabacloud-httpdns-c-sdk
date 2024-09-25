//
// Created by caogaoshuai on 2024/1/18.
//
#include "hdns_log.h"
#include "hdns_resolver.h"

#include "hdns_cache.h"


static char *get_cache_key(const hdns_cache_entry_t *entry) {
    return hdns_str_is_blank(entry->cache_key) ? entry->host : entry->cache_key;
}

static hdns_hash_t *select_hash_table(hdns_cache_t *cache, hdns_rr_type_t type) {
    return type == HDNS_RR_TYPE_A ? cache->v4_table : cache->v6_table;
}


hdns_cache_t *hdns_cache_table_create() {
    hdns_pool_new(pool);
    hdns_cache_t *cache = hdns_palloc(pool, sizeof(hdns_cache_t));
    cache->pool = pool;
    cache->v4_table = apr_hash_make(pool);
    cache->v6_table = apr_hash_make(pool);
    apr_thread_mutex_create(&cache->lock, APR_THREAD_MUTEX_DEFAULT, pool);
    return cache;
}

int32_t hdns_cache_table_add(hdns_cache_t *cache, const hdns_cache_entry_t *entry) {
    apr_thread_mutex_lock(cache->lock);

    apr_hash_t *ht = select_hash_table(cache, entry->type);
    char *cache_key = get_cache_key(entry);

    hdns_cache_entry_t *old_entry = apr_hash_get(ht, cache_key, APR_HASH_KEY_STRING);
    if (old_entry != NULL) {
        apr_hash_set(ht, cache_key, APR_HASH_KEY_STRING, NULL);
        hdns_resv_resp_destroy(old_entry);
    }

    hdns_cache_entry_t *entry_clone = hdns_resv_resp_clone(NULL, entry);
    cache_key = get_cache_key(entry_clone);
    apr_hash_set(ht, cache_key, APR_HASH_KEY_STRING, entry_clone);
    apr_thread_mutex_unlock(cache->lock);
    return HDNS_OK;
}

int32_t hdns_cache_table_delete(hdns_cache_t *cache, const char *key, hdns_rr_type_t type) {
    apr_thread_mutex_lock(cache->lock);
    apr_hash_t *ht = select_hash_table(cache, type);

    hdns_cache_entry_t *old_entry = apr_hash_get(ht, key, APR_HASH_KEY_STRING);
    if (old_entry != NULL) {
        apr_hash_set(ht, get_cache_key(old_entry), APR_HASH_KEY_STRING, NULL);
        hdns_resv_resp_destroy(old_entry);
    }

    apr_thread_mutex_unlock(cache->lock);
    return HDNS_OK;
}

hdns_cache_entry_t *hdns_cache_table_get(hdns_cache_t *cache, const char *key, hdns_rr_type_t type) {
    apr_thread_mutex_lock(cache->lock);
    apr_hash_t *ht = select_hash_table(cache, type);
    hdns_cache_entry_t *entry = apr_hash_get(ht, key, APR_HASH_KEY_STRING);
    if (NULL == entry) {
        hdns_log_debug("cache table get entry failed, entry doesn't exists");
        apr_thread_mutex_unlock(cache->lock);
        return NULL;
    }
    hdns_cache_entry_t *entry_clone = hdns_resv_resp_clone(NULL, entry);
    apr_thread_mutex_unlock(cache->lock);
    return entry_clone;
}

static int hdns_hash_do_get_keys_and_values_callback_fn(void *rec,
                                                        const void *key,
                                                        apr_ssize_t klen,
                                                        const void *value) {
    hdns_unused_var(key);
    hdns_unused_var(klen);
    hdns_list_head_t *list = rec;
    hdns_list_add(list, value, NULL);
    return 1;
}

void hdns_cache_table_clean(hdns_cache_t *cache_table) {
    if (NULL == cache_table) {
        return;
    }
    apr_thread_mutex_lock(cache_table->lock);
    hdns_list_head_t *list = hdns_list_new(NULL);

    apr_hash_do(hdns_hash_do_get_keys_and_values_callback_fn, list, cache_table->v4_table);
    apr_hash_do(hdns_hash_do_get_keys_and_values_callback_fn, list, cache_table->v6_table);
    apr_hash_clear(cache_table->v4_table);
    apr_hash_clear(cache_table->v6_table);
    hdns_list_for_each_entry_safe(cur_cursor, list) {
        hdns_resv_resp_destroy(cur_cursor->data);
    }
    apr_thread_mutex_unlock(cache_table->lock);
    hdns_list_free(list);
}

void hdns_cache_table_cleanup(hdns_cache_t *cache_table) {
    hdns_cache_table_clean(cache_table);
    apr_thread_mutex_destroy(cache_table->lock);
    hdns_pool_destroy(cache_table->pool);
}

static int hdns_hash_do_get_keys_callback_fn(void *rec,
                                             const void *key,
                                             apr_ssize_t klen,
                                             const void *value) {
    hdns_unused_var(key);
    hdns_unused_var(klen);
    hdns_list_head_t *list = rec;
    const hdns_cache_entry_t *entry = value;
    // SNDS entry ignore
    if (hdns_str_is_not_blank(entry->cache_key)
        && hdns_str_is_not_blank(entry->host)
        && strcmp(entry->cache_key, entry->host)) {
        return 1;
    }
    hdns_list_add(list, key, hdns_to_list_clone_fn_t(apr_pstrdup));
    return 1;
}

hdns_list_head_t *hdns_cache_get_keys(hdns_cache_t *cache, hdns_rr_type_t type) {
    hdns_list_head_t *list = hdns_list_new(NULL);
    apr_hash_t *ht = type == HDNS_RR_TYPE_AAAA ? cache->v6_table : cache->v4_table;
    apr_thread_mutex_lock(cache->lock);
    apr_hash_do(hdns_hash_do_get_keys_callback_fn, list, ht);
    apr_thread_mutex_unlock(cache->lock);
    return list;
}