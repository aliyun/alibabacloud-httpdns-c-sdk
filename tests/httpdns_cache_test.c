//
// Created by cagaoshuai on 2024/1/24.
//

#include "httpdns_cache.h"
#include "httpdns_memory.h"
#include "httpdns_time.h"

static httpdns_cache_entry_t* create_test_cache_entry(char* cache_key, int ttl) {
    HTTPDNS_NEW_OBJECT_IN_HEAP(cache_entry, httpdns_cache_entry_t);
    cache_entry->cache_key= sdsnew(cache_key);
    cache_entry->ttl = ttl;
    cache_entry->query_ts = httpdns_time_now();
    httpdns_list_init(&cache_entry->ipsv6);
    httpdns_list_init(&cache_entry->ips);
    return cache_entry;
}

static void test_cache() {
    httpdns_cache_table_t * cache_table = httpdns_cache_table_create();
    httpdns_cache_add_entry(cache_table, create_test_cache_entry("k1.com", 60));
    httpdns_cache_update_entry(cache_table, create_test_cache_entry("k2.com", 60));
    httpdns_cache_table_print(cache_table);
    httpdns_cache_entry_t* entry = create_test_cache_entry("k2.com", 600);
    httpdns_cache_update_entry(cache_table, entry);
    httpdns_cache_table_print(cache_table);
}



int main(void) {
    int32_t success = HTTPDNS_SUCCESS;
    test_cache();
//    success |= test_httpdns_client_simple_resolve();
    return success;
}