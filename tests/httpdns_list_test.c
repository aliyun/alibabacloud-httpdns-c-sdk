//
// Created by cagaoshuai on 2024/1/14.
//

#include<stdio.h>
#include "httpdns_list.h"
#include "sds.h"
#include "httpdns_global_config.h"


static int32_t test_httpdns_list_add() {
    struct list_head list_head;
    httpdns_list_init(&list_head);
    httpdns_list_add(&list_head, "hello", STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "world!", STRING_CLONE_FUNC);
    int32_t ret = HTTPDNS_FAILURE;
    if (&list_head == list_head.next->next->next && list_head.prev == list_head.next->next) {
        ret = HTTPDNS_SUCCESS;
    }
    httpdns_list_free(&list_head, STRING_FREE_FUNC);
    return ret;
}

static int32_t test_httpdns_list_size() {
    struct list_head list_head;
    httpdns_list_init(&list_head);
    httpdns_list_add(&list_head, "hello world!", STRING_CLONE_FUNC);
    int32_t ret = HTTPDNS_FAILURE;
    if (httpdns_list_size(&list_head) == 1) {
        ret = HTTPDNS_SUCCESS;
    }
    httpdns_list_free(&list_head, STRING_FREE_FUNC);
    return ret;
}

static int32_t test_httpdns_list_dup() {
    struct list_head list_head;
    httpdns_list_init(&list_head);
    httpdns_list_add(&list_head, "hello", STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "world!", STRING_CLONE_FUNC);
    struct list_head list_head_dup;
    httpdns_list_dup(&list_head_dup, &list_head, STRING_CLONE_FUNC);
    size_t list_head_size = httpdns_list_size(&list_head);
    size_t list_head_dup_size = httpdns_list_size(&list_head_dup);
    int32_t ret = HTTPDNS_SUCCESS;
    if (list_head_size != list_head_dup_size) {
        ret = HTTPDNS_FAILURE;
    } else {
        for (int i = 0; i < list_head_size; i++) {
            sds list_head_data = httpdns_list_get(&list_head, i);
            sds list_head_dup_data = httpdns_list_get(&list_head_dup, i);
            if (strcmp(list_head_data, list_head_dup_data) != 0) {
                ret = HTTPDNS_FAILURE;
                break;
            }
        }
    }
    httpdns_list_free(&list_head, STRING_FREE_FUNC);
    httpdns_list_free(&list_head_dup, STRING_FREE_FUNC);
    return ret;
}

static int32_t test_httpdns_list_rotate() {
    struct list_head list_head;
    const char *test_data = "world!";
    httpdns_list_init(&list_head);
    httpdns_list_add(&list_head, "hello", STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, test_data, STRING_CLONE_FUNC);
    httpdns_list_rotate(&list_head);
    sds data = httpdns_list_get(&list_head, 0);
    int32_t ret = HTTPDNS_SUCCESS;
    if (strcmp(test_data, data) != 0) {
        ret = HTTPDNS_FAILURE;
    }
    httpdns_list_free(&list_head, STRING_FREE_FUNC);
    return ret;
}


static void test_httpdns_list_shuffle() {
    struct list_head list_head;
    httpdns_list_init(&list_head);
    httpdns_list_add(&list_head, "0", STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "1", STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "2",STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "3",STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "4",STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "5",STRING_CLONE_FUNC);
    size_t list_head_size = httpdns_list_size(&list_head);
    printf("\nbefore shuffle: ");
    for (int i = 0; i < list_head_size; i++) {
        sds list_head_data = httpdns_list_get(&list_head, i);
        printf("%s\t", list_head_data);
    }
    printf("\nafter shuffle: ");
    httpdns_list_shuffle(&list_head);
    for (int i = 0; i < list_head_size; i++) {
        sds list_head_data = httpdns_list_get(&list_head, i);
        printf("%s\t", list_head_data);
    }
    httpdns_list_free(&list_head, STRING_FREE_FUNC);
}

static void test_httpdns_list_sort() {
    struct list_head list_head;
    httpdns_list_init(&list_head);
    httpdns_list_add(&list_head, "5", STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "0", STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "2",STRING_CLONE_FUNC);
    size_t list_head_size = httpdns_list_size(&list_head);
    printf("\nbefore sort: ");
    for (int i = 0; i < list_head_size; i++) {
        sds list_head_data = httpdns_list_get(&list_head, i);
        printf("%s\t", list_head_data);
    }
    printf("\nafter sort: ");
    httpdns_list_sort(&list_head, STRING_CMP_FUNC);
    for (int i = 0; i < list_head_size; i++) {
        sds list_head_data = httpdns_list_get(&list_head, i);
        printf("%s\t", list_head_data);
    }
    httpdns_list_free(&list_head, STRING_FREE_FUNC);
}

static int32_t test_httpdns_list_min() {
    char* excepted_min_val = "0";
    struct list_head list_head;
    httpdns_list_init(&list_head);
    httpdns_list_add(&list_head, "5", STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, excepted_min_val, STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "2", STRING_CLONE_FUNC);
    char* min_val = (char *) httpdns_list_min(&list_head, STRING_CMP_FUNC);
    printf("\nmin: %s", min_val);
    int32_t ret = (strcmp(min_val, excepted_min_val) == 0) ? HTTPDNS_SUCCESS: HTTPDNS_FAILURE;
    httpdns_list_free(&list_head, STRING_FREE_FUNC);
    return ret;
}

static int32_t test_httpdns_list_max() {
    char* excepted_max_val = "5";
    struct list_head list_head;
    httpdns_list_init(&list_head);
    httpdns_list_add(&list_head, excepted_max_val, STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "0", STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "2", STRING_CLONE_FUNC);
    char* max_val = (char *) httpdns_list_max(&list_head, STRING_CMP_FUNC);
    printf("\nmax: %s", max_val);
    int32_t ret =  (strcmp(max_val, excepted_max_val) == 0) ? HTTPDNS_SUCCESS: HTTPDNS_FAILURE;
    httpdns_list_free(&list_head, STRING_FREE_FUNC);
    return ret;
}

static int32_t test_httpdns_list_contain() {
    struct list_head list_head;
    httpdns_list_init(&list_head);
    httpdns_list_add(&list_head, "0",STRING_CLONE_FUNC);
    httpdns_list_add(&list_head, "1",STRING_CLONE_FUNC);
    int32_t ret = HTTPDNS_FAILURE;
    if (httpdns_list_contain(&list_head, "2", STRING_CMP_FUNC) == false) {
        ret = HTTPDNS_SUCCESS;
    }
    if (httpdns_list_contain(&list_head, "0", STRING_CMP_FUNC) == true) {
        ret = HTTPDNS_SUCCESS | ret;
    }
    httpdns_list_free(&list_head, STRING_FREE_FUNC);
    return ret;
}


int main(void) {
    init_httpdns_sdk();
    if (test_httpdns_list_add() != HTTPDNS_SUCCESS) {
        return -1;
    }
    if (test_httpdns_list_size() != HTTPDNS_SUCCESS) {
        return -1;
    }
    if (test_httpdns_list_dup() != HTTPDNS_SUCCESS) {
        return -1;
    }

    if (test_httpdns_list_rotate() != HTTPDNS_SUCCESS) {
        return -1;
    }
    if (test_httpdns_list_contain() != HTTPDNS_SUCCESS) {
        return -1;
    }
    if (test_httpdns_list_min() != HTTPDNS_SUCCESS) {
        return -1;
    }
    if (test_httpdns_list_max() != HTTPDNS_SUCCESS) {
        return -1;
    }

    test_httpdns_list_shuffle();
    test_httpdns_list_sort();
    return 0;
}