//
// Created by caogaoshuai on 2024/1/14.
//

#include "hdns_list.h"
#include "test_suit_list.h"

static size_t hdns_list_string_diff(hdns_list_head_t *list1, hdns_list_head_t *list2) {
    size_t list1_size = hdns_list_size(list1);
    size_t list2_size = hdns_list_size(list2);
    if (list1_size != list2_size) {
        return list1_size;
    }
    int32_t diff_size = 0;
    for (int i = 0; i < list2_size; i++) {
        char *origin_list_data = hdns_list_get(list1, i);
        char *target_list_data = hdns_list_get(list2, i);
        if (strcmp(origin_list_data, target_list_data) != 0) {
            diff_size++;
        }
    }
    return diff_size;
}

void test_list_add(CuTest *tc) {
    hdns_sdk_init();
    hdns_list_head_t *list = hdns_list_new(NULL);
    hdns_list_add(list, "0", NULL);
    hdns_list_add(list, "1", NULL);
    bool is_expected = (list == list->next->next->next && list->prev == list->next->next);

    hdns_list_free(list);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_list_add failed", is_expected);
}

void test_list_rotate(CuTest *tc) {
    hdns_sdk_init();
    hdns_list_head_t *list = hdns_list_new(NULL);
    hdns_list_add(list, "0", NULL);
    hdns_list_add(list, "1", NULL);
    hdns_list_node_t *first_entry = list->next;
    hdns_list_rotate(list);
    bool is_expected = first_entry == list->prev;
    hdns_list_free(list);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_list_rotate failed", is_expected);
}


void test_list_size(CuTest *tc) {
    hdns_sdk_init();
    hdns_list_head_t *list = hdns_list_new(NULL);
    hdns_list_add(list, "0", NULL);
    hdns_list_add(list, "1", NULL);
    bool is_expected = hdns_list_size(list) == 2;
    hdns_list_free(list);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_list_size failed", is_expected);
}


void test_list_dup(CuTest *tc) {
    hdns_sdk_init();
    hdns_list_head_t* origin_list = hdns_list_new(NULL);
    hdns_list_add(origin_list, "0", NULL);
    hdns_list_add(origin_list, "1", NULL);
    hdns_list_head_t *target_list = hdns_list_new(NULL);
    hdns_list_dup(target_list, origin_list, hdns_to_list_clone_fn_t(apr_pstrdup));
    int diff = hdns_list_string_diff(origin_list, target_list);
    hdns_list_free(origin_list);
    hdns_list_free(target_list);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_list_dup failed", diff == 0);
}


void test_list_shuffle(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_list_head_t *origin_list = hdns_list_new(NULL);
    hdns_list_add(origin_list, "0", NULL);
    hdns_list_add(origin_list, "1", NULL);
    hdns_list_add(origin_list, "2", NULL);
    hdns_list_add(origin_list, "3", NULL);
    hdns_list_add(origin_list, "4", NULL);
    hdns_list_add(origin_list, "5", NULL);
    hdns_list_head_t *target_list = hdns_list_new(NULL);
    hdns_list_dup(target_list, origin_list, hdns_to_list_clone_fn_t(apr_pstrdup));
    hdns_list_shuffle(target_list);
    int diff = hdns_list_string_diff(origin_list, target_list);
    hdns_log_debug("diff size is %d", diff);
    hdns_list_free(origin_list);
    hdns_list_free(target_list);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_list_shuffle failed", diff > 0);
}


void test_list_sort(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    hdns_list_head_t *origin_list = hdns_list_new(NULL);
    hdns_list_add(origin_list, "0", NULL);
    hdns_list_add(origin_list, "1", NULL);
    hdns_list_add(origin_list, "2", NULL);
    hdns_list_add(origin_list, "3", NULL);
    hdns_list_add(origin_list, "4", NULL);
    hdns_list_add(origin_list, "5", NULL);
    hdns_list_head_t *target_list = hdns_list_new(NULL);
    hdns_list_dup(target_list, origin_list, hdns_to_list_clone_fn_t(apr_pstrdup));
    hdns_list_shuffle(target_list);

    hdns_list_sort(target_list, hdns_string_cmp_func);

    int diff = hdns_list_string_diff(origin_list, target_list);
    hdns_list_free(origin_list);
    hdns_list_free(target_list);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_list_sort failed", diff == 0);
}


void test_list_min(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    char *excepted_min_val = "0";
    hdns_list_head_t *list = hdns_list_new(NULL);
    hdns_list_add(list, "5", NULL);
    hdns_list_add(list, excepted_min_val, NULL);
    hdns_list_add(list, "2", NULL);
    char *min_val = (char *) hdns_list_min(list, hdns_string_cmp_func);
    hdns_log_debug("test_list_min, min_val=%s", min_val);
    bool is_expected = (strcmp(min_val, excepted_min_val) == 0);
    hdns_list_free(list);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_list_min failed", is_expected);
}


void test_list_max(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    char *excepted_max_val = "5";
    hdns_list_head_t *list = hdns_list_new(NULL);
    hdns_list_add(list, excepted_max_val, NULL);
    hdns_list_add(list, "0", NULL);
    hdns_list_add(list, "2", NULL);
    char *max_val = (char *) hdns_list_max(list, hdns_string_cmp_func);
    hdns_log_debug("test_list_max, max_val=%s", max_val);
    bool is_expected = (strcmp(max_val, excepted_max_val) == 0);
    hdns_list_free(list);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_list_max failed", is_expected);
}


void test_list_contain(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    char *excepted_val = "5";
    hdns_list_head_t *list = hdns_list_new(NULL);
    hdns_list_add(list, "0", NULL);
    hdns_list_add(list, excepted_val, NULL);
    hdns_list_add(list, "1", NULL);
    bool is_expected = hdns_list_contain(list, excepted_val, hdns_string_cmp_func);
    hdns_log_debug("test_list_contain, contain_val=%s", excepted_val);
    hdns_list_free(list);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_list_contain failed", is_expected);
}

static bool str_search_func(const void *data, const void *target) {
    return NULL != strstr(data, target);
}

void test_list_search(CuTest *tc) {
    hdns_sdk_init();
#ifdef TEST_DEBUG_LOG
    hdns_log_level = HDNS_LOG_DEBUG;
#endif
    char *search_target = "5";
    char *expected_val = "500";
    hdns_list_head_t *list = hdns_list_new(NULL);
    hdns_list_add(list, "000", NULL);
    hdns_list_add(list, expected_val, NULL);
    hdns_list_add(list, "123", NULL);
    char *search_result = hdns_list_search(list, search_target, hdns_to_list_search_fn_t(str_search_func));
    bool is_expected = strcmp(expected_val, search_result) == 0;
    hdns_log_debug("test_list_search, search_target=%s, search_result=%s", search_target, search_result);
    hdns_list_free(list);
    hdns_sdk_cleanup();
    CuAssert(tc, "test_list_search failed", is_expected);
}


void add_hdns_list_tests(CuSuite *suite) {
    SUITE_ADD_TEST(suite, test_list_add);
    SUITE_ADD_TEST(suite, test_list_rotate);
    SUITE_ADD_TEST(suite, test_list_size);
    SUITE_ADD_TEST(suite, test_list_dup);
    SUITE_ADD_TEST(suite, test_list_shuffle);
    SUITE_ADD_TEST(suite, test_list_sort);
    SUITE_ADD_TEST(suite, test_list_min);
    SUITE_ADD_TEST(suite, test_list_max);
    SUITE_ADD_TEST(suite, test_list_contain);
    SUITE_ADD_TEST(suite, test_list_search);
}