//
// Created by caogaoshuai on 2024/1/18.
//
#include "httpdns_cache.h"
#include "sds.h"
#include "httpdns_error_type.h"
#include "httpdns_time.h"
#include "httpdns_ip.h"
#include "log.h"
#include "httpdns_string.h"
#include <pthread.h>


httpdns_cache_table_t *httpdns_cache_table_new() {
    HTTPDNS_NEW_OBJECT_IN_HEAP(cache_table, httpdns_cache_table_t);
    cache_table->cache = httpdns_dict_create(&httpdns_dict_type_heap_strings, NULL);
    // 使用递归锁
    pthread_mutexattr_init(&cache_table->lock_attr);
    pthread_mutexattr_settype(&cache_table->lock_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&cache_table->lock, &cache_table->lock_attr);
    return cache_table;
}

int32_t httpdns_cache_table_add(httpdns_cache_table_t *cache_table, httpdns_cache_entry_t *entry) {
    if (NULL == cache_table || NULL == entry || IS_BLANK_STRING(entry->cache_key)) {
        log_info("cache table add entry failed, table or entry or cache_key is NULL");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    pthread_mutex_lock(&cache_table->lock);
    int ret = httpdns_dict_add(cache_table->cache, entry->cache_key, entry);
    pthread_mutex_unlock(&cache_table->lock);
    if (ret != HTTPDNS_DICT_OK) {
        log_info("cache table add entry failed");
        return HTTPDNS_FAILURE;
    }
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_cache_table_delete(httpdns_cache_table_t *cache_table, const char *key) {
    if (NULL == cache_table || NULL == key) {
        log_info("cache table delete entry failed, table or cache_key is NULL");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    pthread_mutex_lock(&cache_table->lock);
    httpdns_cache_entry_t *cache_entry = httpdns_cache_table_get(cache_table, key, NULL);
    if (NULL == cache_entry) {
        pthread_mutex_unlock(&cache_table->lock);
        log_info("cache table delete entry failed, entry doesn't exist");
        return HTTPDNS_FAILURE;
    }
    httpdns_dict_delete(cache_table->cache, key);
    httpdns_cache_entry_free(cache_entry);
    pthread_mutex_unlock(&cache_table->lock);
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_cache_table_update(httpdns_cache_table_t *cache_table, httpdns_cache_entry_t *entry) {
    if (NULL == cache_table || NULL == entry || IS_BLANK_STRING(entry->cache_key)) {
        log_info("cache table update entry failed, table or entry or cache_key is NULL");
        return HTTPDNS_PARAMETER_ERROR;
    }
    pthread_mutex_lock(&cache_table->lock);
    httpdns_cache_entry_t *old_cache_entry = httpdns_cache_table_get(cache_table, entry->cache_key, NULL);
    if (NULL != old_cache_entry) {
        log_debug("old entry exist, update entry");
        if (httpdns_list_is_empty(&old_cache_entry->ips) && httpdns_list_is_not_empty(&entry->ips)) {
            httpdns_list_dup(&old_cache_entry->ips, &entry->ips, to_httpdns_data_clone_func(httpdns_ip_clone));
        }
        if (httpdns_list_is_empty(&old_cache_entry->ipsv6) && httpdns_list_is_not_empty(&entry->ipsv6)) {
            httpdns_list_dup(&old_cache_entry->ipsv6, &entry->ipsv6, to_httpdns_data_clone_func(httpdns_ip_clone));
        }
        old_cache_entry->ttl = entry->ttl;
        old_cache_entry->origin_ttl = entry->origin_ttl;
    } else {
        log_debug("old entry doesn't exist, put new entry into cache table");
        httpdns_cache_entry_t *new_entry = httpdns_resolve_result_clone(entry);
        httpdns_cache_table_add(cache_table, new_entry);
    }
    pthread_mutex_unlock(&cache_table->lock);
    return HTTPDNS_SUCCESS;
}

httpdns_cache_entry_t *
httpdns_cache_table_get(httpdns_cache_table_t *cache_table, const char *key, const char *dns_type) {
    if (NULL == cache_table || NULL == key) {
        log_info("cache table get entry failed, table or cache_key is NULL");
        return NULL;
    }
    pthread_mutex_lock(&cache_table->lock);
    // 存在
    httpdns_dict_entry_t *dict_entry = httpdns_dict_find(cache_table->cache, key);
    if (NULL == dict_entry) {
        log_debug("cache table get entry failed, entry doesn't exists");
        pthread_mutex_unlock(&cache_table->lock);
        return NULL;
    }
    // 过期
    httpdns_cache_entry_t *entry = dict_entry->val;
    int ttl = entry->origin_ttl > 0 ? entry->origin_ttl : entry->ttl;
    if (httpdns_time_is_expired(entry->query_ts, ttl)) {
        httpdns_dict_delete(cache_table->cache, key);
        httpdns_cache_entry_free(entry);
        log_debug("cache table get entry failed, entry is expired");
        pthread_mutex_unlock(&cache_table->lock);
        return NULL;
    }
    // 类型
    if (IS_TYPE_A(dns_type) && httpdns_list_is_empty(&entry->ips)) {
        log_debug("cache table get entry failed, ips is empty");
        pthread_mutex_unlock(&cache_table->lock);
        return NULL;
    }
    if (IS_TYPE_AAAA(dns_type) && httpdns_list_is_empty(&entry->ipsv6)) {
        log_debug("cache table get entry failed, ipsv6 is empty");
        pthread_mutex_unlock(&cache_table->lock);
        return NULL;
    }
    pthread_mutex_unlock(&cache_table->lock);
    return entry;
}

void httpdns_cache_table_clean(httpdns_cache_table_t *cache_table) {
    if (NULL == cache_table) {
        return;
    }
    pthread_mutex_lock(&cache_table->lock);
    httpdns_dict_iterator_t *di = httpdns_dict_get_safe_iterator(cache_table->cache);
    if (NULL == di) {
        pthread_mutex_unlock(&cache_table->lock);
        return;
    }
    httpdns_dict_entry_t *de = NULL;
    while ((de = httpdns_dict_next(di)) != NULL) {
        char *cache_key = (char *) de->key;
        log_debug("delete cache entry %s", cache_key);
        httpdns_cache_table_delete(cache_table, cache_key);
    }
    httpdns_dict_release_iterator(di);
    pthread_mutex_unlock(&cache_table->lock);
}

sds httpdns_cache_table_to_string(httpdns_cache_table_t *cache_table) {
    if (NULL == cache_table) {
        return sdsnew("cache_table()");
    }
    pthread_mutex_lock(&cache_table->lock);
    httpdns_dict_iterator_t *di = httpdns_dict_get_safe_iterator(cache_table->cache);
    if (NULL == di) {
        pthread_mutex_unlock(&cache_table->lock);
        return sdsnew("cache_table()");
    }
    sds dst_str = sdsnew("cache_table(");
    httpdns_dict_entry_t *de = NULL;
    while ((de = httpdns_dict_next(di)) != NULL) {
        SDS_CAT(dst_str, "\t");
        httpdns_cache_entry_t *entry = (httpdns_cache_entry_t *) de->val;
        sds entry_str = httpdns_resolve_result_to_string(entry);
        SDS_CAT(dst_str, entry_str);
        sdsfree(entry_str);
    }
    httpdns_dict_release_iterator(di);
    SDS_CAT(dst_str, ")");
    pthread_mutex_unlock(&cache_table->lock);
    return dst_str;
}

void httpdns_cache_table_free(httpdns_cache_table_t *cache_table) {
    if (NULL != cache_table) {
        httpdns_cache_table_clean(cache_table);
        httpdns_dict_release(cache_table->cache);
        pthread_mutex_destroy(&cache_table->lock);
        pthread_mutexattr_destroy(&cache_table->lock_attr);
        free(cache_table);
    }
}

void httpdns_cache_entry_free(httpdns_cache_entry_t *entry) {
    if (NULL != entry) {
        httpdns_resolve_result_free(entry);
    }
}

void httpdns_cache_entry_rotate(httpdns_cache_entry_t *cache_entry) {
    if (NULL != cache_entry) {
        httpdns_list_rotate(&cache_entry->ips);
        httpdns_list_rotate(&cache_entry->ipsv6);
    }
}