//
// Created by caogaoshuai on 2024/1/14.
//

#include "httpdns_list.h"
#include "sds.h"
#include "check_suit_list.h"
#include "httpdns_global_config.h"

static void setup(void) {
    init_httpdns_sdk();
}

static void teardown(void) {
    cleanup_httpdns_sdk();
}

static size_t httpdns_list_string_diff(struct list_head *list1, struct list_head *list2) {
    size_t list1_size = httpdns_list_size(list1);
    size_t list2_size = httpdns_list_size(list2);
    if (list1_size != list2_size) {
        return list1_size;
    }
    int32_t diff_size = 0;
    for (int i = 0; i < list2_size; i++) {
        sds origin_list_data = httpdns_list_get(list1, i);
        sds target_list_data = httpdns_list_get(list2, i);
        if (strcmp(origin_list_data, target_list_data) != 0) {
            diff_size++;
        }
    }
    return diff_size;
}

START_TEST(test_list_add) {
    NEW_EMPTY_LIST_IN_STACK(list);
    httpdns_list_add(&list, "0", STRING_CLONE_FUNC);
    httpdns_list_add(&list, "1", STRING_CLONE_FUNC);
    bool is_expected = (&list == list.next->next->next && list.prev == list.next->next);
    httpdns_list_free(&list, STRING_FREE_FUNC);
    ck_assert_msg(is_expected, "链表指针关系不符合预期");
}

END_TEST

START_TEST(test_list_rotate) {
    NEW_EMPTY_LIST_IN_STACK(list);
    httpdns_list_add(&list, "0", STRING_CLONE_FUNC);
    httpdns_list_add(&list, "1", STRING_CLONE_FUNC);
    struct list_head *first_entry = list.next;
    httpdns_list_rotate(&list);
    bool is_expected = first_entry == list.prev;
    httpdns_list_free(&list, STRING_FREE_FUNC);
    ck_assert_msg(is_expected, "链表指针关系不符合预期");
}

END_TEST

START_TEST(test_list_size) {
    NEW_EMPTY_LIST_IN_STACK(list);
    httpdns_list_add(&list, "0", STRING_CLONE_FUNC);
    httpdns_list_add(&list, "1", STRING_CLONE_FUNC);
    bool is_expected = httpdns_list_size(&list) == 2;
    httpdns_list_free(&list, STRING_FREE_FUNC);
    ck_assert_msg(is_expected, "链表size不等于2");
}

END_TEST


START_TEST(test_list_dup) {
    NEW_EMPTY_LIST_IN_STACK(origin_list);
    httpdns_list_add(&origin_list, "0", STRING_CLONE_FUNC);
    httpdns_list_add(&origin_list, "1", STRING_CLONE_FUNC);
    NEW_EMPTY_LIST_IN_STACK(target_list);
    httpdns_list_dup(&target_list, &origin_list, STRING_CLONE_FUNC);
    bool is_expected = httpdns_list_string_diff(&origin_list, &target_list) == 0;
    httpdns_list_free(&origin_list, STRING_FREE_FUNC);
    httpdns_list_free(&target_list, STRING_FREE_FUNC);
    ck_assert_msg(is_expected, "链表复制不符合预期，数据不一致");
}

END_TEST


START_TEST(test_list_shuffle) {
    NEW_EMPTY_LIST_IN_STACK(origin_list);
    httpdns_list_add(&origin_list, "0", STRING_CLONE_FUNC);
    httpdns_list_add(&origin_list, "1", STRING_CLONE_FUNC);
    httpdns_list_add(&origin_list, "2", STRING_CLONE_FUNC);
    httpdns_list_add(&origin_list, "3", STRING_CLONE_FUNC);
    httpdns_list_add(&origin_list, "4", STRING_CLONE_FUNC);
    httpdns_list_add(&origin_list, "5", STRING_CLONE_FUNC);
    NEW_EMPTY_LIST_IN_STACK(target_list);
    httpdns_list_dup(&target_list, &origin_list, STRING_CLONE_FUNC);
    httpdns_list_shuffle(&target_list);
    bool is_expected = httpdns_list_string_diff(&origin_list, &target_list) > 0;
    sds origin_list_str = httpdns_list_to_string(&origin_list, NULL);
    sds target_list_str = httpdns_list_to_string(&target_list, NULL);
    log_trace("test_list_shuffle, before shuffle list=%s, after shuffle list=%s", origin_list_str, target_list_str);
    sdsfree(origin_list_str);
    sdsfree(target_list_str);
    httpdns_list_free(&origin_list, STRING_FREE_FUNC);
    httpdns_list_free(&target_list, STRING_FREE_FUNC);
    ck_assert_msg(is_expected, "链表打乱失败");
}

END_TEST

START_TEST(test_list_sort) {
    NEW_EMPTY_LIST_IN_STACK(origin_list);
    httpdns_list_add(&origin_list, "0", STRING_CLONE_FUNC);
    httpdns_list_add(&origin_list, "1", STRING_CLONE_FUNC);
    httpdns_list_add(&origin_list, "2", STRING_CLONE_FUNC);
    httpdns_list_add(&origin_list, "3", STRING_CLONE_FUNC);
    httpdns_list_add(&origin_list, "4", STRING_CLONE_FUNC);
    httpdns_list_add(&origin_list, "5", STRING_CLONE_FUNC);
    NEW_EMPTY_LIST_IN_STACK(target_list);
    httpdns_list_dup(&target_list, &origin_list, STRING_CLONE_FUNC);
    httpdns_list_shuffle(&target_list);
    sds shuffle_list_str = httpdns_list_to_string(&target_list, NULL);
    httpdns_list_sort(&target_list, STRING_CMP_FUNC);
    sds target_list_str = httpdns_list_to_string(&target_list, NULL);
    log_trace("test_list_sort, before sort list=%s, after sort list=%s", shuffle_list_str, target_list_str);
    sdsfree(shuffle_list_str);
    sdsfree(target_list_str);
    bool is_expected = httpdns_list_string_diff(&origin_list, &target_list) == 0;
    httpdns_list_free(&origin_list, STRING_FREE_FUNC);
    httpdns_list_free(&target_list, STRING_FREE_FUNC);
    ck_assert_msg(is_expected, "链表未按照从小到大排序");
}

END_TEST


START_TEST(test_list_min) {
    char *excepted_min_val = "0";
    NEW_EMPTY_LIST_IN_STACK(list_head);
    httpdns_list_add(&list_head, "5", STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, excepted_min_val, STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "2", STRING_CLONE_FUNC);
    char *min_val = (char *) httpdns_list_min(&list_head, STRING_CMP_FUNC);
    sds list_str = httpdns_list_to_string(&list_head, NULL);
    log_trace("test_list_min, list_str=%s, min_val=%s", list_str, min_val);
    sdsfree(list_str);
    bool is_expected = (strcmp(min_val, excepted_min_val) == 0);
    httpdns_list_free(&list_head, STRING_FREE_FUNC);
    ck_assert_msg(is_expected, "链表搜索最小值不符合预期");
}

END_TEST

START_TEST(test_list_max) {
    char *excepted_max_val = "5";
    NEW_EMPTY_LIST_IN_STACK(list_head);
    httpdns_list_add(&list_head, excepted_max_val, STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "0", STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "2", STRING_CLONE_FUNC);
    char *max_val = (char *) httpdns_list_max(&list_head, STRING_CMP_FUNC);
    sds list_str = httpdns_list_to_string(&list_head, NULL);
    log_trace("test_list_max, list_str=%s, max_val=%s", list_str, max_val);
    sdsfree(list_str);
    bool is_expected = (strcmp(max_val, excepted_max_val) == 0);
    httpdns_list_free(&list_head, STRING_FREE_FUNC);
    ck_assert_msg(is_expected, "链表搜索最大值不符合预期");
}

END_TEST

START_TEST(test_list_contain) {
    char *excepted_val = "5";
    NEW_EMPTY_LIST_IN_STACK(list_head);
    httpdns_list_add(&list_head, "0", STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, excepted_val, STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "1", STRING_CLONE_FUNC);
    bool is_expected = httpdns_list_contain(&list_head, excepted_val, STRING_CMP_FUNC);
    sds list_str = httpdns_list_to_string(&list_head, NULL);
    log_trace("test_list_contain, list_str=%s, contain_val=%s", list_str, excepted_val);
    sdsfree(list_str);
    httpdns_list_free(&list_head, STRING_FREE_FUNC);
    ck_assert_msg(is_expected, "链表包含判定错误");
}

static bool str_search_func(const void *data, const void *target) {
    return NULL != strstr(data, target);
}

START_TEST(test_list_search) {
    char *search_target = "5";
    char *expected_val = "500";
    NEW_EMPTY_LIST_IN_STACK(list_head);
    httpdns_list_add(&list_head, "000", STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, expected_val, STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "123", STRING_CLONE_FUNC);
    char *search_result = httpdns_list_search(&list_head, search_target, DATA_SEARCH_FUNC(str_search_func));
    bool is_expected = strcmp(expected_val, search_result) == 0;
    sds list_str = httpdns_list_to_string(&list_head, NULL);
    log_trace("test_list_search, list_str=%s, search_target=%s, search_result=%s", list_str, search_target,
              search_result);
    sdsfree(list_str);
    httpdns_list_free(&list_head, STRING_FREE_FUNC);
    ck_assert_msg(is_expected, "链表搜索结果不符合预期");
}


Suite *make_httpdns_list_suite(void) {
    Suite *suite = suite_create("HTTPDNS List Test");
    TCase *httpdns_list = tcase_create("httpdns_list");
    tcase_add_unchecked_fixture(httpdns_list, setup, teardown);
    suite_add_tcase(suite, httpdns_list);
    tcase_add_test(httpdns_list, test_list_add);
    tcase_add_test(httpdns_list, test_list_rotate);
    tcase_add_test(httpdns_list, test_list_size);
    tcase_add_test(httpdns_list, test_list_dup);
    tcase_add_test(httpdns_list, test_list_shuffle);
    tcase_add_test(httpdns_list, test_list_sort);
    tcase_add_test(httpdns_list, test_list_min);
    tcase_add_test(httpdns_list, test_list_max);
    tcase_add_test(httpdns_list, test_list_contain);
    tcase_add_test(httpdns_list, test_list_search);
    return suite;
}