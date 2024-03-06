//
// Created by caogaoshuai on 2024/1/24.
//

#include "httpdns_cache.h"
#include "httpdns_memory.h"
#include "httpdns_time.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"
#include <unistd.h>

static httpdns_cache_entry_t *create_test_cache_entry(char *cache_key, int ttl) {
    httpdns_new_object_in_heap(cache_entry, httpdns_cache_entry_t);
    cache_entry->cache_key = httpdns_sds_new(cache_key);
    cache_entry->ttl = ttl;
    cache_entry->query_ts = httpdns_time_now();
    httpdns_list_init(&cache_entry->ipsv6);
    httpdns_list_init(&cache_entry->ips);
    return cache_entry;
}

void test_miss_cache(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_cache_table_t *cache_table = httpdns_cache_table_new();
    httpdns_cache_table_add(cache_table, create_test_cache_entry("k1.com", 60));
    httpdns_cache_entry_t *entry = httpdns_cache_table_get(cache_table, "k2.com", NULL);
    bool is_miss_cache = (entry == NULL);
    httpdns_cache_table_free(cache_table);
    cleanup_httpdns_sdk();
    CuAssert(tc, "非预期命中到缓存", is_miss_cache);
}

void test_hit_cache(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_cache_table_t *cache_table = httpdns_cache_table_new();
    httpdns_cache_table_add(cache_table, create_test_cache_entry("k1.com", 60));
    httpdns_cache_entry_t *entry = httpdns_cache_table_get(cache_table, "k1.com", NULL);
    bool is_hit_cache = (entry != NULL);
    httpdns_cache_table_free(cache_table);
    cleanup_httpdns_sdk();
    CuAssert(tc, "非预期缓存缺失", is_hit_cache);
}

void test_cache_expired(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_cache_table_t *cache_table = httpdns_cache_table_new();
    httpdns_cache_table_add(cache_table, create_test_cache_entry("k1.com", 1));
    sleep(2);
    httpdns_cache_entry_t *entry = httpdns_cache_table_get(cache_table, "k1.com", NULL);
    bool is_miss_cache = (entry == NULL);
    httpdns_cache_table_free(cache_table);
    cleanup_httpdns_sdk();
    CuAssert(tc, "非预期命中过期缓存", is_miss_cache);
}

void test_delete_cache_entry(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_cache_table_t *cache_table = httpdns_cache_table_new();
    httpdns_cache_table_add(cache_table, create_test_cache_entry("k1.com", 60));
    httpdns_sds_t cache_str = httpdns_cache_table_to_string(cache_table);
    httpdns_log_trace("test_delete_cache_entry, cache table=%s", cache_str);
    httpdns_sds_free(cache_str);
    httpdns_cache_table_delete(cache_table, "k1.com");
    httpdns_cache_entry_t *entry = httpdns_cache_table_get(cache_table, "k1.com", NULL);
    bool is_miss_cache = (entry == NULL);
    httpdns_cache_table_free(cache_table);
    cleanup_httpdns_sdk();
    CuAssert(tc, "非预期命中过期缓存", is_miss_cache);
}

void test_update_cache_entry(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_cache_table_t *cache_table = httpdns_cache_table_new();
    httpdns_cache_table_add(cache_table, create_test_cache_entry("k1.com", 60));
    httpdns_cache_entry_t *update_entry = create_test_cache_entry("k1.com", 80);
    update_entry->origin_ttl = 120;
    httpdns_cache_table_update(cache_table, update_entry);
    httpdns_cache_entry_t *entry = httpdns_cache_table_get(cache_table, "k1.com", NULL);
    bool is_expected = (NULL != entry && entry->ttl == 80 && entry->origin_ttl == 120);
    httpdns_cache_table_free(cache_table);
    httpdns_resolve_result_free(update_entry);
    cleanup_httpdns_sdk();
    CuAssert(tc, "更新缓存失败", is_expected);
}

void test_clean_cache(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_cache_table_t *cache_table = httpdns_cache_table_new();
    httpdns_cache_table_add(cache_table, create_test_cache_entry("k1.com", 60));
    httpdns_cache_table_clean(cache_table);
    httpdns_cache_entry_t *entry = httpdns_cache_table_get(cache_table, "k1.com", NULL);
    bool is_expected = (NULL == entry);
    httpdns_cache_table_free(cache_table);
    cleanup_httpdns_sdk();
    CuAssert(tc, "清理缓存失败", is_expected);
}


CuSuite *make_httpdns_cache_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_miss_cache);
    SUITE_ADD_TEST(suite, test_hit_cache);
    SUITE_ADD_TEST(suite, test_cache_expired);
    SUITE_ADD_TEST(suite, test_delete_cache_entry);
    SUITE_ADD_TEST(suite, test_update_cache_entry);
    SUITE_ADD_TEST(suite, test_clean_cache);
    return suite;
}