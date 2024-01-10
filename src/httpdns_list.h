//
// Created by cagaoshuai on 2024/1/9.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H

#include "../libs/list.h"
#include "httpdns_error_type.h"
#include <stdlib.h>
#include <string.h>

typedef struct _httpdns_list_node {
    struct list_head list;
    void *data;
} httpdns_list_node_t;

void httpdns_list_init(struct list_head *head);

int32_t httpdns_list_add(struct list_head *head, void *data);

int32_t httpdns_list_rotate(struct list_head *head);

struct list_head *httpdns_list_dup(struct list_head *dst_head, struct list_head *src_head);

httpdns_list_node_t *httpdns_list_get(struct list_head *head, int index);

size_t httpdns_list_size(struct list_head *head);

void httpdns_list_free(struct list_head *head);


#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H
