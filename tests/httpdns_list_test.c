//
// Created by cagaoshuai on 2024/1/14.
//

#include<stdio.h>
#include "httpdns_list.h"
#include "sds.h"
#include "httpdns_global_init.h"


static int32_t _test_httpdns_list_add() {
    struct list_head list_head;
    httpdns_list_init(&list_head);
    httpdns_list_add(&list_head, sdsnew("hello"));
    httpdns_list_add(&list_head, sdsnew("world!"));
    int32_t ret = HTTPDNS_FAILURE;
    if (&list_head == list_head.next->next->next && list_head.prev == list_head.next->next) {
        ret = HTTPDNS_SUCCESS;
    }
    httpdns_list_free(&list_head, (data_free_function_ptr_t) sdsfree);
    return ret;
}

static int32_t _test_httpdns_list_size() {
    struct list_head list_head;
    httpdns_list_init(&list_head);
    httpdns_list_add(&list_head, sdsnew("hello world!"));
    int32_t ret = HTTPDNS_FAILURE;
    if (httpdns_list_size(&list_head) == 1) {
        ret = HTTPDNS_SUCCESS;
    }
    httpdns_list_free(&list_head, (data_free_function_ptr_t) sdsfree);
    return ret;
}

static int32_t _test_httpdns_list_dup() {
    struct list_head list_head;
    httpdns_list_init(&list_head);
    httpdns_list_add(&list_head, sdsnew("hello"));
    httpdns_list_add(&list_head, sdsnew("world!"));
    struct list_head list_head_dup;
    httpdns_list_dup(&list_head_dup, &list_head, (data_clone_function_ptr_t) sdsnew);
    size_t list_head_size = httpdns_list_size(&list_head);
    size_t list_head_dup_size = httpdns_list_size(&list_head_dup);
    if (list_head_size != list_head_dup_size) {
        return HTTPDNS_FAILURE;
    }
    for (int i = 0; i < list_head_size; i++) {
        sds list_head_data = httpdns_list_get(&list_head, i);
        sds list_head_dup_data = httpdns_list_get(&list_head_dup, i);
        if (strcmp(list_head_data, list_head_dup_data) != 0) {
            return HTTPDNS_FAILURE;
        }
    }
    httpdns_list_free(&list_head, (data_free_function_ptr_t) sdsfree);
    httpdns_list_free(&list_head_dup, (data_free_function_ptr_t) sdsfree);
    return HTTPDNS_SUCCESS;
}

static int32_t _test_httpdns_list_rotate() {
    struct list_head list_head;
    const char* test_data = "world!";
    httpdns_list_init(&list_head);
    httpdns_list_add(&list_head, sdsnew("hello"));
    httpdns_list_add(&list_head, sdsnew(test_data));
    httpdns_list_rotate(&list_head);
    sds data = httpdns_list_get(&list_head, 0);
    if (strcmp(test_data, data) != 0) {
        return HTTPDNS_SUCCESS;
    }
    return HTTPDNS_SUCCESS;
}


static int32_t _test_httpdns_list_shuffle() {
    struct list_head list_head;
    httpdns_list_init(&list_head);
    httpdns_list_add(&list_head, sdsnew("0"));
    httpdns_list_add(&list_head, sdsnew("1"));
    httpdns_list_add(&list_head, sdsnew("2"));
    httpdns_list_add(&list_head, sdsnew("3"));
    httpdns_list_add(&list_head, sdsnew("4"));
    httpdns_list_add(&list_head, sdsnew("5"));
    size_t list_head_size = httpdns_list_size(&list_head);
    printf("before shuffle: ");
    for (int i = 0; i < list_head_size; i++) {
        sds list_head_data = httpdns_list_get(&list_head, i);
        printf("%s\t", list_head_data);
    }
    printf("after shuffle: ");
    httpdns_list_shuffle(&list_head);
    for (int i = 0; i < list_head_size; i++) {
        sds list_head_data = httpdns_list_get(&list_head, i);
        printf("%s\t", list_head_data);
    }
    return HTTPDNS_SUCCESS;
}


int main(int argc, char *argv[]) {
    init_httpdns_sdk();
    if (_test_httpdns_list_add() != HTTPDNS_SUCCESS) {
        return -1;
    }
    if (_test_httpdns_list_size() != HTTPDNS_SUCCESS) {
        return -1;
    }
    if (_test_httpdns_list_dup() != HTTPDNS_SUCCESS) {
        return -1;
    }

    if (_test_httpdns_list_rotate() != HTTPDNS_SUCCESS) {
        return -1;
    }
    if (_test_httpdns_list_shuffle() != HTTPDNS_SUCCESS) {
        return -1;
    }
    return 0;
}