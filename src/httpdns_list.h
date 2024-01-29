//
// Created by cagaoshuai on 2024/1/9.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H

#include "list.h"
#include "httpdns_error_type.h"
#include "httpdns_memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "sds.h"

#define NEW_EMPTY_LIST_IN_STACK(var_name) \
    struct list_head var_name;       \
    httpdns_list_init(&var_name)

#define IS_EMPTY_LIST(list) \
    (NULL == list || httpdns_list_size(list) <=0)

#define IS_NOT_EMPTY_LIST(list) \
    (NULL != list || httpdns_list_size(list) >0)

#define DATA_CLONE_FUNC(func) \
   (data_clone_function_ptr_t)func

#define DATA_CMP_FUNC(func) \
   (data_cmp_function_ptr_t) func

#define DATA_FREE_FUNC(func) \
   (data_free_function_ptr_t)func

#define DATA_TO_STRING_FUNC(func) \
   (data_to_string_function_ptr_t)func

#define DATA_SEARCH_FUNC(func) \
   (data_search_function_ptr_t)func

#define STRING_CLONE_FUNC \
   DATA_CLONE_FUNC(sdsnew)

#define STRING_CMP_FUNC \
    DATA_CMP_FUNC(strcmp)

#define STRING_FREE_FUNC \
  DATA_FREE_FUNC(sdsfree)


#define httpdns_list_for_each_entry(cursor, head) \
            httpdns_list_node_t *cursor;      \
            list_for_each_entry(cursor, head, list)


typedef struct {
    struct list_head list;
    void *data;
} httpdns_list_node_t;

typedef void (*data_free_function_ptr_t )(const void *data);

typedef void *(*data_clone_function_ptr_t )(const void *data);

typedef int32_t (*data_cmp_function_ptr_t)(const void *data1, const void *data2);

typedef bool (*data_search_function_ptr_t)(const void *data, const void *target);

typedef sds (*data_to_string_function_ptr_t )(const void *data);

void httpdns_list_init(struct list_head *head);

/**
 *
 * @param head
 * @param data don't free data after httpdns_list_add, data will free when httpdns_list_free
 * @return
 */
int32_t httpdns_list_add(struct list_head *head, const void *data, data_clone_function_ptr_t clone_func);

int32_t httpdns_list_rotate(struct list_head *head);

struct list_head *
httpdns_list_dup(struct list_head *dst_head, struct list_head *src_head, data_clone_function_ptr_t clone_func);

void *httpdns_list_get(struct list_head *head, int index);

size_t httpdns_list_size(struct list_head *head);

void httpdns_list_free(struct list_head *head, data_free_function_ptr_t free_func);

void httpdns_list_shuffle(struct list_head *head);

bool httpdns_list_contain(struct list_head *head, const void *data, data_cmp_function_ptr_t cmp_func);

void *httpdns_list_min(struct list_head *head, data_cmp_function_ptr_t cmp_func);

void *httpdns_list_max(struct list_head *head, data_cmp_function_ptr_t cmp_func);

void httpdns_list_sort(struct list_head *head, data_cmp_function_ptr_t cmp_func);

sds httpdns_list_to_string(struct list_head *head, data_to_string_function_ptr_t to_string_func);

void *httpdns_list_search(struct list_head *head, const void *target, data_search_function_ptr_t search_func);

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H
