//
// Created by caogaoshuai on 2024/1/9.
// 参考linux list.h
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "httpdns_error_type.h"
#include "httpdns_memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "httpdns_sds.h"

#define httpdns_list_new_empty_in_stack(var_name) \
    httpdns_list_head_t var_name;       \
    httpdns_list_init(&var_name)

#define httpdns_list_is_empty(list) \
    (NULL == list || httpdns_list_size(list) <=0)

#define httpdns_list_is_not_empty(list) \
    (NULL != list || httpdns_list_size(list) >0)

#define to_httpdns_data_clone_func(func) \
   (httpdns_data_clone_func_t)func

#define to_httpdns_data_cmp_func(func) \
   (httpdns_data_cmp_func_t) func

#define to_httpdns_data_free_func(func) \
   (httpdns_data_free_func_t)func

#define to_httpdns_data_to_string_func(func) \
   (httpdns_data_to_string_func_t)func

#define to_httpdns_data_search_func(func) \
   (httpdns_data_search_func_t)func

#define httpdns_string_clone_func \
   to_httpdns_data_clone_func(httpdns_sds_new)

#define httpdns_string_cmp_func \
    to_httpdns_data_cmp_func(strcmp)

#define httpdns_string_free_func \
  to_httpdns_data_free_func(httpdns_sds_free)


#define httpdns_list_for_each_entry(cursor, head) \
    for (httpdns_list_node_t *cursor = httpdns_list_first_entry(head); cursor != head; cursor = cursor->next)


typedef struct httpdns_list_node_t {
    struct httpdns_list_node_t *next, *prev;
    void *data;
} httpdns_list_node_t;

typedef httpdns_list_node_t httpdns_list_head_t;

typedef void (*httpdns_data_free_func_t )(const void *data);

typedef void *(*httpdns_data_clone_func_t )(const void *data);

typedef int32_t (*httpdns_data_cmp_func_t)(const void *data1, const void *data2);

typedef bool (*httpdns_data_search_func_t)(const void *data, const void *target);

typedef char* (*httpdns_data_to_string_func_t )(const void *data);

void httpdns_list_init(httpdns_list_head_t *head);

httpdns_list_node_t* httpdns_list_first_entry(httpdns_list_head_t *head);

int32_t httpdns_list_add(httpdns_list_head_t *head, const void *data, httpdns_data_clone_func_t clone_func);

int32_t httpdns_list_rotate(httpdns_list_head_t *head);

void httpdns_list_dup(httpdns_list_head_t *dst_head, const httpdns_list_head_t *src_head,
                      httpdns_data_clone_func_t clone_func);

void *httpdns_list_get(const httpdns_list_head_t *head, int index);

size_t httpdns_list_size(const httpdns_list_head_t *head);

void httpdns_list_free(httpdns_list_head_t *head, httpdns_data_free_func_t free_func);

void httpdns_list_shuffle(httpdns_list_head_t *head);

bool httpdns_list_contain(const httpdns_list_head_t *head, const void *data, httpdns_data_cmp_func_t cmp_func);

void *httpdns_list_min(const httpdns_list_head_t *head, httpdns_data_cmp_func_t cmp_func);

void *httpdns_list_max(const httpdns_list_head_t *head, httpdns_data_cmp_func_t cmp_func);

void httpdns_list_sort(httpdns_list_head_t *head, httpdns_data_cmp_func_t cmp_func);

httpdns_sds_t httpdns_list_to_string(const httpdns_list_head_t *head, httpdns_data_to_string_func_t to_string_func);

void *httpdns_list_search(const httpdns_list_head_t *head, const void *target, httpdns_data_search_func_t search_func);

bool httpdns_list_is_end_node(const httpdns_list_node_t *node, const httpdns_list_head_t *head);

#ifdef __cplusplus
}
#endif


#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H
