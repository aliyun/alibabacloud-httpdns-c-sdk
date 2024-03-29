//
// Created by caogaoshuai on 2024/1/14.
//

#include "httpdns_list.h"
#include "httpdns_sds.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"

static size_t httpdns_list_string_diff(httpdns_list_head_t *list1, httpdns_list_head_t *list2) {
    size_t list1_size = httpdns_list_size(list1);
    size_t list2_size = httpdns_list_size(list2);
    if (list1_size != list2_size) {
        return list1_size;
    }
    int32_t diff_size = 0;
    for (int i = 0; i < list2_size; i++) {
        httpdns_sds_t origin_list_data = httpdns_list_get(list1, i);
        httpdns_sds_t target_list_data = httpdns_list_get(list2, i);
        if (strcmp(origin_list_data, target_list_data) != 0) {
            diff_size++;
        }
    }
    return diff_size;
}

void test_list_add(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_list_new_empty_in_stack(list);
    httpdns_list_add(&list, "0", httpdns_string_clone_func);
    httpdns_list_add(&list, "1", httpdns_string_clone_func);
    bool is_expected = (&list == list.next->next->next && list.prev == list.next->next);
    httpdns_list_free(&list, httpdns_string_free_func);
    cleanup_httpdns_sdk();
    CuAssert(tc, "链表指针关系不符合预期", is_expected);
}

void test_list_rotate(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_list_new_empty_in_stack(list);
    httpdns_list_add(&list, "0", httpdns_string_clone_func);
    httpdns_list_add(&list, "1", httpdns_string_clone_func);
    httpdns_list_node_t *first_entry = list.next;
    httpdns_list_rotate(&list);
    bool is_expected = first_entry == list.prev;
    httpdns_list_free(&list, httpdns_string_free_func);
    cleanup_httpdns_sdk();
    CuAssert(tc, "链表指针关系不符合预期", is_expected);
}


void test_list_size(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_list_new_empty_in_stack(list);
    httpdns_list_add(&list, "0", httpdns_string_clone_func);
    httpdns_list_add(&list, "1", httpdns_string_clone_func);
    bool is_expected = httpdns_list_size(&list) == 2;
    httpdns_list_free(&list, httpdns_string_free_func);
    cleanup_httpdns_sdk();
    CuAssert(tc, "链表size不等于2", is_expected);
}


void test_list_dup(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_list_new_empty_in_stack(origin_list);
    httpdns_list_add(&origin_list, "0", httpdns_string_clone_func);
    httpdns_list_add(&origin_list, "1", httpdns_string_clone_func);
    httpdns_list_new_empty_in_stack(target_list);
    httpdns_list_dup(&target_list, &origin_list, httpdns_string_clone_func);
    bool is_expected = httpdns_list_string_diff(&origin_list, &target_list) == 0;
    httpdns_list_free(&origin_list, httpdns_string_free_func);
    httpdns_list_free(&target_list, httpdns_string_free_func);
    cleanup_httpdns_sdk();
    CuAssert(tc, "链表复制不符合预期，数据不一致", is_expected);
}


void test_list_shuffle(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_list_new_empty_in_stack(origin_list);
    httpdns_list_add(&origin_list, "0", httpdns_string_clone_func);
    httpdns_list_add(&origin_list, "1", httpdns_string_clone_func);
    httpdns_list_add(&origin_list, "2", httpdns_string_clone_func);
    httpdns_list_add(&origin_list, "3", httpdns_string_clone_func);
    httpdns_list_add(&origin_list, "4", httpdns_string_clone_func);
    httpdns_list_add(&origin_list, "5", httpdns_string_clone_func);
    httpdns_list_new_empty_in_stack(target_list);
    httpdns_list_dup(&target_list, &origin_list, httpdns_string_clone_func);
    httpdns_list_shuffle(&target_list);
    bool is_expected = httpdns_list_string_diff(&origin_list, &target_list) > 0;
    httpdns_sds_t origin_list_str = httpdns_list_to_string(&origin_list, NULL);
    httpdns_sds_t target_list_str = httpdns_list_to_string(&target_list, NULL);
    httpdns_log_trace("test_list_shuffle, before shuffle list=%s, after shuffle list=%s", origin_list_str,
                      target_list_str);
    httpdns_sds_free(origin_list_str);
    httpdns_sds_free(target_list_str);
    httpdns_list_free(&origin_list, httpdns_string_free_func);
    httpdns_list_free(&target_list, httpdns_string_free_func);
    cleanup_httpdns_sdk();
    CuAssert(tc, "链表打乱失败", is_expected);
}


void test_list_sort(CuTest *tc) {
    init_httpdns_sdk();
    httpdns_list_new_empty_in_stack(origin_list);
    httpdns_list_add(&origin_list, "0", httpdns_string_clone_func);
    httpdns_list_add(&origin_list, "1", httpdns_string_clone_func);
    httpdns_list_add(&origin_list, "2", httpdns_string_clone_func);
    httpdns_list_add(&origin_list, "3", httpdns_string_clone_func);
    httpdns_list_add(&origin_list, "4", httpdns_string_clone_func);
    httpdns_list_add(&origin_list, "5", httpdns_string_clone_func);
    httpdns_list_new_empty_in_stack(target_list);
    httpdns_list_dup(&target_list, &origin_list, httpdns_string_clone_func);
    httpdns_list_shuffle(&target_list);
    httpdns_sds_t shuffle_list_str = httpdns_list_to_string(&target_list, NULL);
    httpdns_list_sort(&target_list, httpdns_string_cmp_func);
    httpdns_sds_t target_list_str = httpdns_list_to_string(&target_list, NULL);
    httpdns_log_trace("test_list_sort, before sort list=%s, after sort list=%s", shuffle_list_str, target_list_str);
    httpdns_sds_free(shuffle_list_str);
    httpdns_sds_free(target_list_str);
    bool is_expected = httpdns_list_string_diff(&origin_list, &target_list) == 0;
    httpdns_list_free(&origin_list, httpdns_string_free_func);
    httpdns_list_free(&target_list, httpdns_string_free_func);
    cleanup_httpdns_sdk();
    CuAssert(tc, "链表未按照从小到大排序", is_expected);
}


void test_list_min(CuTest *tc) {
    init_httpdns_sdk();
    char *excepted_min_val = "0";
    httpdns_list_new_empty_in_stack(list_head);
    httpdns_list_add(&list_head, "5", httpdns_string_clone_func);
    httpdns_list_add(&list_head, excepted_min_val, httpdns_string_clone_func);
    httpdns_list_add(&list_head, "2", httpdns_string_clone_func);
    char *min_val = (char *) httpdns_list_min(&list_head, httpdns_string_cmp_func);
    httpdns_sds_t list_str = httpdns_list_to_string(&list_head, NULL);
    httpdns_log_trace("test_list_min, list_str=%s, min_val=%s", list_str, min_val);
    httpdns_sds_free(list_str);
    bool is_expected = (strcmp(min_val, excepted_min_val) == 0);
    httpdns_list_free(&list_head, httpdns_string_free_func);
    cleanup_httpdns_sdk();
    CuAssert(tc, "链表搜索最小值不符合预期", is_expected);
}


void test_list_max(CuTest *tc) {
    init_httpdns_sdk();
    char *excepted_max_val = "5";
    httpdns_list_new_empty_in_stack(list_head);
    httpdns_list_add(&list_head, excepted_max_val, httpdns_string_clone_func);
    httpdns_list_add(&list_head, "0", httpdns_string_clone_func);
    httpdns_list_add(&list_head, "2", httpdns_string_clone_func);
    char *max_val = (char *) httpdns_list_max(&list_head, httpdns_string_cmp_func);
    httpdns_sds_t list_str = httpdns_list_to_string(&list_head, NULL);
    httpdns_log_trace("test_list_max, list_str=%s, max_val=%s", list_str, max_val);
    httpdns_sds_free(list_str);
    bool is_expected = (strcmp(max_val, excepted_max_val) == 0);
    httpdns_list_free(&list_head, httpdns_string_free_func);
    cleanup_httpdns_sdk();
    CuAssert(tc, "链表搜索最大值不符合预期", is_expected);
}


void test_list_contain(CuTest *tc) {
    init_httpdns_sdk();
    char *excepted_val = "5";
    httpdns_list_new_empty_in_stack(list_head);
    httpdns_list_add(&list_head, "0", httpdns_string_clone_func);
    httpdns_list_add(&list_head, excepted_val, httpdns_string_clone_func);
    httpdns_list_add(&list_head, "1", httpdns_string_clone_func);
    bool is_expected = httpdns_list_contain(&list_head, excepted_val, httpdns_string_cmp_func);
    httpdns_sds_t list_str = httpdns_list_to_string(&list_head, NULL);
    httpdns_log_trace("test_list_contain, list_str=%s, contain_val=%s", list_str, excepted_val);
    httpdns_sds_free(list_str);
    httpdns_list_free(&list_head, httpdns_string_free_func);
    cleanup_httpdns_sdk();
    CuAssert(tc, "链表包含判定错误", is_expected);
}

static bool str_search_func(const void *data, const void *target) {
    return NULL != strstr(data, target);
}

void test_list_search(CuTest *tc) {
    init_httpdns_sdk();
    char *search_target = "5";
    char *expected_val = "500";
    httpdns_list_new_empty_in_stack(list_head);
    httpdns_list_add(&list_head, "000", httpdns_string_clone_func);
    httpdns_list_add(&list_head, expected_val, httpdns_string_clone_func);
    httpdns_list_add(&list_head, "123", httpdns_string_clone_func);
    char *search_result = httpdns_list_search(&list_head, search_target, to_httpdns_data_search_func(str_search_func));
    bool is_expected = strcmp(expected_val, search_result) == 0;
    httpdns_sds_t list_str = httpdns_list_to_string(&list_head, NULL);
    httpdns_log_trace("test_list_search, list_str=%s, search_target=%s, search_result=%s", list_str, search_target,
                      search_result);
    httpdns_sds_free(list_str);
    httpdns_list_free(&list_head, httpdns_string_free_func);
    cleanup_httpdns_sdk();
    CuAssert(tc, "链表搜索结果不符合预期", is_expected);
}


CuSuite *make_httpdns_list_suite(void) {
    CuSuite *suite = CuSuiteNew();
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
    return suite;
}