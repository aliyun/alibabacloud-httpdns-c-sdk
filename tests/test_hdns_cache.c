//
// Created by caogaoshuai on 2024/1/24.
//
#include <unistd.h>
#include "hdns_cache.h"

#include "test_suit_list.h"


static hdns_cache_entry_t *create_test_cache_entry(hdns_cache_t *cache, char *cache_key, int ttl) {
    hdns_pool_new_with_pp(pool, cache->pool);
    hdns_cache_entry_t *entry = hdns_resv_resp_create_empty(pool, NULL, HDNS_RR_TYPE_A);
    entry->cache_key = apr_pstrdup(pool, cache_key);
    entry->ttl = ttl;
    return entry;
}

void test_miss_cache(CuTest *tc) {
    hdns_sdk_init();
    hdns_client_t *client = hdns_client_create("139450", NULL);
    hdns_cache_table_add(client->cache, create_test_cache_entry(client->cache, "k1.com", 60));
    hdns_cache_entry_t *entry = hdns_cache_table_get(client->cache, "k2.com", HDNS_RR_TYPE_A);
    bool is_miss_cache = (entry == NULL);
    hdns_resv_resp_destroy(entry);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "非预期命中到缓存", is_miss_cache);
}

void test_hit_cache(CuTest *tc) {
    hdns_sdk_init();
    hdns_client_t *client = hdns_client_create("139450", NULL);
    hdns_cache_table_add(client->cache, create_test_cache_entry(client->cache, "k1.com", 60));
    hdns_cache_entry_t *entry = hdns_cache_table_get(client->cache, "k1.com", HDNS_RR_TYPE_A);
    bool is_hit_cache = (entry != NULL);
    hdns_resv_resp_destroy(entry);
    hdns_sdk_cleanup();
    CuAssert(tc, "非预期缓存缺失", is_hit_cache);
}

void test_cache_expired(CuTest *tc) {
    hdns_sdk_init();
    hdns_client_t *client = hdns_client_create("139450", NULL);
    hdns_cache_table_add(client->cache, create_test_cache_entry(client->cache, "k1.com", 1));
    sleep(2);
    hdns_cache_entry_t *entry = hdns_cache_table_get(client->cache, "k1.com", HDNS_RR_TYPE_A);
    bool is_miss_cache = (entry == NULL);
    hdns_resv_resp_destroy(entry);
    hdns_sdk_cleanup();
    CuAssert(tc, "非预期命中过期缓存", is_miss_cache);
}

void test_delete_cache_entry(CuTest *tc) {
    hdns_sdk_init();
    hdns_client_t *client = hdns_client_create("139450", NULL);
    hdns_cache_table_add(client->cache, create_test_cache_entry(client->cache, "k1.com", 60));
    hdns_cache_table_delete(client->cache, "k1.com", HDNS_RR_TYPE_A);
    hdns_cache_entry_t *entry = hdns_cache_table_get(client->cache, "k1.com", HDNS_RR_TYPE_A);
    bool is_miss_cache = (entry == NULL);
    hdns_resv_resp_destroy(entry);
    hdns_sdk_cleanup();
    CuAssert(tc, "非预期命中过期缓存", is_miss_cache);
}

void test_update_cache_entry(CuTest *tc) {
    hdns_sdk_init();
    hdns_client_t *client = hdns_client_create("139450", NULL);
    hdns_cache_table_add(client->cache, create_test_cache_entry(client->cache, "k1.com", 60));
    hdns_cache_entry_t *update_entry = create_test_cache_entry(client->cache, "k1.com", 80);
    update_entry->origin_ttl = 120;
    hdns_cache_table_add(client->cache, update_entry);
    hdns_cache_entry_t *entry = hdns_cache_table_get(client->cache, "k1.com", HDNS_RR_TYPE_A);
    bool is_expected = (NULL != entry && entry->ttl == 80 && entry->origin_ttl == 120);
    hdns_resv_resp_destroy(entry);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "更新缓存失败", is_expected);
}

void test_clean_cache(CuTest *tc) {
    hdns_sdk_init();
    hdns_client_t *client = hdns_client_create("139450", NULL);
    hdns_cache_table_add(client->cache, create_test_cache_entry(client->cache, "k1.com", 60));
    hdns_cache_table_clean(client->cache);
    hdns_cache_entry_t *entry = hdns_cache_table_get(client->cache, "k1.com", HDNS_RR_TYPE_A);
    bool is_expected = (NULL == entry);
    hdns_resv_resp_destroy(entry);
    hdns_client_cleanup(client);
    hdns_sdk_cleanup();
    CuAssert(tc, "清理缓存失败", is_expected);
}


void add_hdns_cache_tests(CuSuite *suite) {
    SUITE_ADD_TEST(suite, test_miss_cache);
    SUITE_ADD_TEST(suite, test_hit_cache);
    SUITE_ADD_TEST(suite, test_delete_cache_entry);
    SUITE_ADD_TEST(suite, test_update_cache_entry);
}