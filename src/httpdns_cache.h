//
// Created by caogaoshuai on 2024/1/10.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_CACHE_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_CACHE_H

#include "dict.h"
#include "httpdns_resolver.h"
#include "httpdns_resolve_result.h"

typedef struct {
    dict *cache;
    pthread_mutex_t lock;
    pthread_mutexattr_t lock_attr;
} httpdns_cache_table_t;

typedef httpdns_resolve_result_t httpdns_cache_entry_t;

void httpdns_cache_entry_free(httpdns_cache_entry_t *entry);

httpdns_cache_table_t *httpdns_cache_table_new();

int32_t httpdns_cache_table_add(httpdns_cache_table_t *cache_table, httpdns_cache_entry_t *entry);

int32_t httpdns_cache_table_delete(httpdns_cache_table_t *cache_table, const char *key);

int32_t httpdns_cache_table_update(httpdns_cache_table_t *cache_table, httpdns_cache_entry_t *entry);

httpdns_cache_entry_t *httpdns_cache_table_get(httpdns_cache_table_t *cache_table, const char *key, const char *dns_type);

void httpdns_cache_table_clean(httpdns_cache_table_t *cache_table);

sds httpdns_cache_table_to_string(httpdns_cache_table_t *cache_table);

void httpdns_cache_entry_rotate(httpdns_cache_entry_t *cache_entry);

void httpdns_cache_table_free(httpdns_cache_table_t *cache_table);

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_CACHE_H
