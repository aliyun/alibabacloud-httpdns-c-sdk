//
// Created by cagaoshuai on 2024/1/10.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_CACHE_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_CACHE_H

#include "dict.h"
#include "httpdns_result.h"

typedef dict httpdns_cache_table_t;

typedef httpdns_resolve_result_t httpdns_cache_entry_t;

void destroy_httpdns_cache_entry(httpdns_cache_entry_t* entry);

httpdns_cache_table_t *create_httpdns_cache_table();

int32_t httpdns_cache_add_entry(httpdns_cache_table_t *cache_table, httpdns_cache_entry_t *entry);

int32_t httpdns_cache_delete_entry(httpdns_cache_table_t *cache_table, char *key);

int32_t httpdns_cache_update_entry(httpdns_cache_table_t *cache_table, httpdns_cache_entry_t *entry);

httpdns_cache_entry_t *httpdns_cache_get_entry(httpdns_cache_table_t *cache_table, char *key);

void httpdns_cache_clean_cache(httpdns_cache_table_t *cache_table);

void httpdns_cache_table_print(httpdns_cache_table_t *cache_table);

void httpdns_cache_entry_print(httpdns_cache_entry_t *cache_entry);

void destroy_httpdns_cache_table(httpdns_cache_table_t *cache_table);

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_CACHE_H
