//
// Created by cagaoshuai on 2024/1/18.
//
#include "httpdns_cache.h"
#include "sds.h"
#include "httpdns_error_type.h"
#include "httpdns_time.h"

httpdns_cache_table_t *create_httpdns_cache_table() {
    return dictCreate(&dictTypeHeapStrings, NULL);
}

int32_t httpdns_cache_add_entry(httpdns_cache_table_t *cache_table, httpdns_cache_entry_t *entry) {
    if (NULL == entry || IS_BLANK_SDS(entry->cache_key)) {
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
    httpdns_cache_entry_t *cache_entry = httpdns_cache_get_entry(cache_table, key);
    if (NULL != cache_entry) {
        dictDelete(cache_table, key);
        destroy_httpdns_cache_entry(cache_entry);
        return HTTPDNS_SUCCESS;
    }
    return HTTPDNS_FAILURE;
}

int32_t httpdns_cache_update_entry(httpdns_cache_table_t *cache_table, httpdns_cache_entry_t *entry) {
    if (NULL == cache_table || NULL == entry || IS_BLANK_SDS(entry->cache_key)) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    httpdns_cache_delete_entry(cache_table, entry->cache_key);
    return httpdns_cache_add_entry(cache_table, entry);
}

httpdns_cache_entry_t *httpdns_cache_get_entry(httpdns_cache_table_t *cache_table, char *key) {
    if (NULL == cache_table || NULL == key) {
        return NULL;
    }
    dictEntry *dict_entry = dictFind(cache_table, key);
    if (NULL == dict_entry) {
        return NULL;
    }
    httpdns_cache_entry_t *entry = dict_entry->val;
    int ttl = entry->origin_ttl > 0 ? entry->origin_ttl : entry->ttl;
    if (httpdns_time_is_expired(entry->query_ts, ttl)) {
        dictDelete(cache_table, key);
        destroy_httpdns_cache_entry(entry);
        return NULL;
    }
    return dict_entry->val;
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
                httpdns_cache_entry_print(entry);
            }
            dictReleaseIterator(di);
            printf("]");
        } else {
            printf("\nCache=[]");
        }
    } else {
        printf("\nCache=[]");
    }
}

void httpdns_cache_entry_print(httpdns_cache_entry_t *cache_entry) {
    if (NULL == cache_entry) {
        printf("{ null }");
        return;
    }
    if (NULL != cache_entry) {
        printf("\ncache entry:");
        printf("{ ");
        printf("host=%s,", cache_entry->host);
        printf("client_ip=%s,", cache_entry->client_ip);
        if (IS_NOT_BLANK_SDS(cache_entry->extra)) {
            printf("extra=%s,", cache_entry->extra);
        }
        printf("origin_ttl=%d,", cache_entry->origin_ttl);
        printf("ttl=%d,", cache_entry->ttl);
        char buffer[256];
        httpdns_time_to_string(cache_entry->query_ts, buffer, 256);
        printf("query_timestamp=%s,", buffer);
        printf("cache_key=%s", cache_entry->cache_key);
        printf(" }");
    }
}

void destroy_httpdns_cache_table(httpdns_cache_table_t *cache) {
    if (NULL != cache) {
        httpdns_cache_clean_cache(cache);
        dictRelease(cache);
    }
}

void destroy_httpdns_cache_entry(httpdns_cache_entry_t *entry) {
    destroy_httpdns_resolve_result(entry);
}