//
// Created by cagaoshuai on 2024/1/9.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H

#include "list.h"
#include "httpdns_error_type.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define IS_EMPTY_LIST(list) \
 (NULL == list || httpdns_list_size(list) <=0)

typedef struct {
    struct list_head list;
    void *data;
} httpdns_list_node_t;

typedef void (*data_free_function_ptr_t )(void *data);

typedef void *(*data_clone_function_ptr_t )(void *data);

typedef uint32_t (*data_cmp_function_ptr_t)(void *data1, void *data2);

void httpdns_list_init(struct list_head *head);

/**
 *
 * @param head
 * @param data don't free data after httpdns_list_add, data will free when httpdns_list_free
 * @return
 */
int32_t httpdns_list_add(struct list_head *head, void *data);

int32_t httpdns_list_rotate(struct list_head *head);

struct list_head *
httpdns_list_dup(struct list_head *dst_head, struct list_head *src_head, data_clone_function_ptr_t clone_func);

void *httpdns_list_get(struct list_head *head, int index);

size_t httpdns_list_size(struct list_head *head);

void httpdns_list_free(struct list_head *head, data_free_function_ptr_t free_func);

void httpdns_list_shuffle(struct list_head *head);

bool httpdns_list_contain(struct list_head *head, void *data, data_cmp_function_ptr_t);


#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H
