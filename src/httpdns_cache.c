//
// Created by cagaoshuai on 2024/1/18.
//
#include "httpdns_cache.h"
#include "sds.h"
#include "httpdns_error_type.h"
#include "httpdns_time.h"
#include "httpdns_ip.h"

httpdns_cache_table_t *httpdns_cache_table_create() {
    return dictCreate(&dictTypeHeapStrings, NULL);
}

int32_t httpdns_cache_add_entry(httpdns_cache_table_t *cache_table, httpdns_cache_entry_t *entry) {
    if (NULL == entry || IS_BLANK_STRING(entry->cache_key)) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (dictAdd(cache_table, entry->cache_key, entry) != DICT_OK) {
        return HTTPDNS_FAILURE;
    }
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_cache_delete_entry(httpdns_cache_table_t *cache_table, char *key) {
    if (NULL == cache_table || NULL == key) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    httpdns_cache_entry_t *cache_entry = httpdns_cache_get_entry(cache_table, key, NULL);
    if (NULL != cache_entry) {
        dictDelete(cache_table, key);
        httpdns_cache_destroy_entry(cache_entry);
        return HTTPDNS_SUCCESS;
    }
    return HTTPDNS_FAILURE;
}

int32_t httpdns_cache_update_entry(httpdns_cache_table_t *cache_table, httpdns_cache_entry_t *entry) {
    if (NULL == cache_table || NULL == entry || IS_BLANK_STRING(entry->cache_key)) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    httpdns_cache_entry_t *old_cache_entry = httpdns_cache_get_entry(cache_table, entry->cache_key, NULL);
    if (NULL != old_cache_entry) {
        if (IS_EMPTY_LIST(&old_cache_entry->ips) && IS_NOT_EMPTY_LIST(&entry->ips)) {
            httpdns_list_dup(&old_cache_entry->ips, &entry->ips, DATA_CLONE_FUNC(httpdns_ip_clone));
        }
        if (IS_EMPTY_LIST(&old_cache_entry->ipsv6) && IS_NOT_EMPTY_LIST(&entry->ipsv6)) {
            httpdns_list_dup(&old_cache_entry->ipsv6, &entry->ipsv6, DATA_CLONE_FUNC(httpdns_ip_clone));
        }
        old_cache_entry->ttl = entry->ttl;
        old_cache_entry->origin_ttl = entry->origin_ttl;
    } else {
        httpdns_cache_entry_t *new_entry = httpdns_resolve_result_clone(entry);
        httpdns_cache_add_entry(cache_table, new_entry);
    }
    return HTTPDNS_SUCCESS;
}

httpdns_cache_entry_t *httpdns_cache_get_entry(httpdns_cache_table_t *cache_table, char *key, char *dns_type) {
    if (NULL == cache_table || NULL == key) {
        return NULL;
    }
    // 存在
    dictEntry *dict_entry = dictFind(cache_table, key);
    if (NULL == dict_entry) {
        return NULL;
    }
    // 过期
    httpdns_cache_entry_t *entry = dict_entry->val;
    int ttl = entry->origin_ttl > 0 ? entry->origin_ttl : entry->ttl;
    if (httpdns_time_is_expired(entry->query_ts, ttl)) {
        dictDelete(cache_table, key);
        httpdns_cache_destroy_entry(entry);
        return NULL;
    }
    // 类型
    if (IS_TYPE_A(dns_type) && IS_EMPTY_LIST(&entry->ips)) {
        return NULL;
    }
    if (IS_TYPE_AAAA(dns_type) && IS_EMPTY_LIST(&entry->ipsv6)) {
        return NULL;
    }
    return entry;
}

void httpdns_cache_clean_cache(httpdns_cache_table_t *cache_table) {
    if (NULL != cache_table) {
        dictIterator *di = dictGetSafeIterator(cache_table);
        if (NULL != di) {
            dictEntry *de = NULL;
            while ((de = dictNext(di)) != NULL) {
                char *cache_key = (char *) de->key;
                httpdns_cache_delete_entry(cache_table, cache_key);
            }
            dictReleaseIterator(di);
        }
    }
}

void httpdns_cache_table_print(httpdns_cache_table_t *cache_table) {
    if (NULL != cache_table) {
        dictIterator *di = dictGetSafeIterator(cache_table);
        if (NULL != di) {
            printf("Cache=[");
            dictEntry *de = NULL;
            while ((de = dictNext(di)) != NULL) {
                httpdns_cache_entry_t *entry = (httpdns_cache_entry_t *) de->val;
                printf("\n");
                httpdns_cache_print_entry(entry);
            }
            dictReleaseIterator(di);
            printf("\n]");
        } else {
            printf("\nCache=[]");
        }
    } else {
        printf("\nCache=[]");
    }
}

void httpdns_cache_print_entry(httpdns_cache_entry_t *cache_entry) {
    httpdns_resolve_result_print(cache_entry);
}

void httpdns_cache_table_destroy(httpdns_cache_table_t *cache_table) {
    if (NULL != cache_table) {
        httpdns_cache_clean_cache(cache_table);
        dictRelease(cache_table);
    }
}

void httpdns_cache_destroy_entry(httpdns_cache_entry_t *entry) {
    if (NULL != entry) {
        httpdns_resolve_result_destroy(entry);
    }
}

void httpdns_cache_rotate_entry(httpdns_cache_entry_t *cache_entry) {
    if (NULL != cache_entry) {
        httpdns_list_rotate(&cache_entry->ips);
        httpdns_list_rotate(&cache_entry->ipsv6);
    }
}