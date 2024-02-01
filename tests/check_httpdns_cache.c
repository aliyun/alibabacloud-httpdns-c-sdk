//
// Created by cagaoshuai on 2024/1/24.
//

#include "httpdns_cache.h"
#include "httpdns_memory.h"
#include "httpdns_time.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"

static void setup(void) {
    init_httpdns_sdk();
}

static void teardown(void) {
    cleanup_httpdns_sdk();
}

static httpdns_cache_entry_t *create_test_cache_entry(char *cache_key, int ttl) {
    HTTPDNS_NEW_OBJECT_IN_HEAP(cache_entry, httpdns_cache_entry_t);
    cache_entry->cache_key = sdsnew(cache_key);
    cache_entry->ttl = ttl;
    cache_entry->query_ts = httpdns_time_now();
    httpdns_list_init(&cache_entry->ipsv6);
    httpdns_list_init(&cache_entry->ips);
    return cache_entry;
}

START_TEST(test_miss_cache) {
    httpdns_cache_table_t *cache_table = httpdns_cache_table_new();
    httpdns_cache_table_add(cache_table, create_test_cache_entry("k1.com", 60));
    httpdns_cache_entry_t *entry = httpdns_cache_table_get(cache_table, "k2.com", NULL);
    bool is_miss_cache = (entry == NULL);
    httpdns_cache_table_free(cache_table);
    ck_assert_msg(is_miss_cache, "非预期命中到缓存");
}

START_TEST(test_hit_cache) {
    httpdns_cache_table_t *cache_table = httpdns_cache_table_new();
    httpdns_cache_table_add(cache_table, create_test_cache_entry("k1.com", 60));
    httpdns_cache_entry_t *entry = httpdns_cache_table_get(cache_table, "k1.com", NULL);
    bool is_hit_cache = (entry != NULL);
    httpdns_cache_table_free(cache_table);
    ck_assert_msg(is_hit_cache, "非预期缓存缺失");
}

START_TEST(test_cache_expired) {
    httpdns_cache_table_t *cache_table = httpdns_cache_table_new();
    httpdns_cache_table_add(cache_table, create_test_cache_entry("k1.com", 1));
    sleep(2);
    httpdns_cache_entry_t *entry = httpdns_cache_table_get(cache_table, "k1.com", NULL);
    bool is_miss_cache = (entry == NULL);
    httpdns_cache_table_free(cache_table);
    ck_assert_msg(is_miss_cache, "非预期命中过期缓存");
}

START_TEST(test_delete_cache_entry) {
    httpdns_cache_table_t *cache_table = httpdns_cache_table_new();
    httpdns_cache_table_add(cache_table, create_test_cache_entry("k1.com", 60));
    sds cache_str = httpdns_cache_table_to_string(cache_table);
    log_trace("test_delete_cache_entry, cache table=%s", cache_str);
    sdsfree(cache_str);
    httpdns_cache_table_delete(cache_table, "k1.com");
    httpdns_cache_entry_t *entry = httpdns_cache_table_get(cache_table, "k1.com", NULL);
    bool is_miss_cache = (entry == NULL);
    httpdns_cache_table_free(cache_table);
    ck_assert_msg(is_miss_cache, "非预期命中过期缓存");
}

START_TEST(test_update_cache_entry) {
    httpdns_cache_table_t *cache_table = httpdns_cache_table_new();
    httpdns_cache_table_add(cache_table, create_test_cache_entry("k1.com", 60));
    httpdns_cache_entry_t *update_entry = create_test_cache_entry("k1.com", 80);
    update_entry->origin_ttl = 120;
    httpdns_cache_table_update(cache_table, update_entry);
    httpdns_cache_entry_t *entry = httpdns_cache_table_get(cache_table, "k1.com", NULL);
    bool is_expected = (NULL != entry && entry->ttl == 80 && entry->origin_ttl == 120);
    httpdns_cache_table_free(cache_table);
    httpdns_resolve_result_free(update_entry);
    ck_assert_msg(is_expected, "更新缓存失败");
}

START_TEST(test_clean_cache) {
    httpdns_cache_table_t *cache_table = httpdns_cache_table_new();
    httpdns_cache_table_add(cache_table, create_test_cache_entry("k1.com", 60));
    httpdns_cache_table_clean(cache_table);
    httpdns_cache_entry_t *entry = httpdns_cache_table_get(cache_table, "k1.com", NULL);
    bool is_expected = (NULL == entry);
    httpdns_cache_table_free(cache_table);
    ck_assert_msg(is_expected, "清理缓存失败");
}


Suite *make_httpdns_cache_suite(void) {
    Suite *suite = suite_create("HTTPDNS Cache Test");
    TCase *httpdns_cache = tcase_create("httpdns_cache");
    tcase_add_unchecked_fixture(httpdns_cache, setup, teardown);
    suite_add_tcase(suite, httpdns_cache);
    tcase_add_test(httpdns_cache, test_miss_cache);
    tcase_add_test(httpdns_cache, test_hit_cache);
    tcase_add_test(httpdns_cache, test_cache_expired);
    tcase_add_test(httpdns_cache, test_delete_cache_entry);
    tcase_add_test(httpdns_cache, test_update_cache_entry);
    tcase_add_test(httpdns_cache, test_clean_cache);
    return suite;
}