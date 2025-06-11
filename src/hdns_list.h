//
// Created by caogaoshuai on 2024/1/9.
// 参考linux list.h
//

#ifndef HDNS_C_SDK_HDNS_LIST_H
#define HDNS_C_SDK_HDNS_LIST_H

#include "hdns_define.h"

HDNS_CPP_START


#define hdns_list_is_empty(list) \
    (NULL == list || hdns_list_size(list) <=0)

#define hdns_list_is_not_empty(list) \
    (NULL != list && hdns_list_size(list) >0)

#define hdns_to_list_clone_fn_t(func) \
   (hdns_list_clone_fn_t)func

#define hdns_to_list_filter_fn_t(func) \
   (hdns_list_filter_fn_t)func

#define hdns_to_list_cmp_fn_t(func) \
   (hdns_list_cmp_pt) func

#define hdns_to_list_search_fn_t(func) \
   (hdns_list_search_pt)func

#define hdns_string_cmp_func \
    hdns_to_list_cmp_fn_t(strcmp)


#define hdns_list_for_each_entry(cursor, head) \
    for (hdns_list_node_t *cursor = hdns_list_first(head); cursor != head; cursor = cursor->next)


#define hdns_list_for_each_entry_safe(cur_cursor, head) \
    for (hdns_list_node_t *cur_cursor = hdns_list_first(head), *next_cursor = cur_cursor->next; cur_cursor != head; cur_cursor = next_cursor, next_cursor=cur_cursor->next)

typedef struct hdns_list_node_s hdns_list_node_t;

struct hdns_list_node_s {
    hdns_pool_t *pool;
    hdns_list_node_t *next, *prev;
    void *data;
};

typedef hdns_list_node_t hdns_list_head_t;

#if defined(_WIN32) && defined(_M_IX86)
typedef void* (__stdcall* hdns_list_clone_stdcall_fn_t)(hdns_pool_t* pool, const void* data);
#endif

typedef void *(*hdns_list_clone_fn_t )(hdns_pool_t *pool, const void *data);

typedef bool (*hdns_list_filter_fn_t )(const void *data);

typedef int32_t (*hdns_list_cmp_pt)(const void *data1, const void *data2);

typedef bool (*hdns_list_search_pt)(const void *data, const void *target);

hdns_list_head_t *hdns_list_new(hdns_pool_t *pool);

void hdns_list_free(hdns_list_head_t *list);

int32_t hdns_list_add(hdns_list_head_t *head, const void *data, hdns_list_clone_fn_t clone);

hdns_list_node_t *hdns_list_first(const hdns_list_head_t *head);

void hdns_list_del(hdns_list_node_t *entry);

int32_t hdns_list_rotate(hdns_list_head_t *head);

void hdns_list_dup(hdns_list_head_t *dst_head,
                   const hdns_list_head_t *src_head,
                   hdns_list_clone_fn_t clone);

void hdns_list_filter(hdns_list_head_t *dst_head,
                      const hdns_list_head_t *src_head,
                      hdns_list_clone_fn_t clone,
                      hdns_list_filter_fn_t filter);

void *hdns_list_get(const hdns_list_head_t *head, int index);

size_t hdns_list_size(const hdns_list_head_t *head);

void hdns_list_shuffle(hdns_list_head_t *head);

bool hdns_list_contain(const hdns_list_head_t *head, const void *data, hdns_list_cmp_pt cmp_func);

void *hdns_list_min(const hdns_list_head_t *head, hdns_list_cmp_pt cmp_func);

void *hdns_list_max(const hdns_list_head_t *head, hdns_list_cmp_pt cmp_func);

void hdns_list_sort(hdns_list_head_t *head, hdns_list_cmp_pt cmp_func);

void *hdns_list_search(const hdns_list_head_t *head, const void *target, hdns_list_search_pt search_func);

bool hdns_list_is_end_node(const hdns_list_node_t *node, const hdns_list_head_t *head);

void hdns_list_insert_tail(hdns_list_node_t *_new, hdns_list_head_t *head);

HDNS_CPP_END

#endif
