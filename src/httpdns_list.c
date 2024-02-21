//
// Created by caogaoshuai on 2024/1/10.
//
#include "httpdns_list.h"
#include "httpdns_sds.h"
#include <stdio.h>

void httpdns_list_init(httpdns_list_head_t *head) {
    if (NULL == head) {
        return;
    }
    head->next = head;
    head->prev = head;
}

static inline void httpdns_list_insert(httpdns_list_node_t *_new,
                                       httpdns_list_node_t *prev,
                                       httpdns_list_node_t *next) {
    next->prev = _new;
    _new->next = next;
    _new->prev = prev;
    prev->next = _new;
}

static inline void httpdns_list_insert_head(httpdns_list_node_t *_new, httpdns_list_head_t *head) {
    httpdns_list_insert(_new, head, head->next);
}

static inline void httpdns_list_insert_tail(httpdns_list_node_t *_new, httpdns_list_head_t *head) {
    httpdns_list_insert(_new, head->prev, head);
}

int32_t httpdns_list_add(httpdns_list_head_t *head, const void *data, httpdns_data_clone_func_t clone_func) {
    if (NULL == head || NULL == data) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    size_t size = sizeof(httpdns_list_node_t);
    httpdns_list_node_t *node = malloc(size);
    if (NULL != node) {
        memset(node, 0, size);
        if (NULL == clone_func) {
            node->data = (void *) data;
        } else {
            node->data = clone_func(data);
        }
        httpdns_list_insert_tail(node, head);
        return HTTPDNS_SUCCESS;
    }
    return HTTPDNS_MEMORY_ALLOCATION_ERROR;
}

httpdns_list_node_t *httpdns_list_first_entry(httpdns_list_head_t *head) {
    return head->next;
}

static inline void httpdns_list_del_init(httpdns_list_node_t *entry) {
    if (NULL == entry) {
        return;
    }
    httpdns_list_node_t *next = entry->next;
    httpdns_list_node_t *pre = entry->prev;
    pre->next = next;
    next->prev = pre;
}

int32_t httpdns_list_rotate(httpdns_list_head_t *head) {
    if (NULL == head || httpdns_list_size(head) == 0) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (httpdns_list_size(head) == 1) {
        return HTTPDNS_SUCCESS;
    }
    httpdns_list_node_t *first_entry = httpdns_list_first_entry(head);
    httpdns_list_del_init(first_entry);
    httpdns_list_insert_tail(first_entry, head);
    return HTTPDNS_SUCCESS;
}


void httpdns_list_dup(httpdns_list_head_t *dst_head, const httpdns_list_head_t *src_head,
                      httpdns_data_clone_func_t clone_func) {
    if (NULL == dst_head || NULL == src_head || NULL == clone_func) {
        return;
    }
    httpdns_list_init(dst_head);
    for (httpdns_list_node_t *cursor = httpdns_list_first_entry(src_head); cursor != src_head; cursor = cursor->next) {
        httpdns_list_add(dst_head, cursor->data, clone_func);
    }
}

void *httpdns_list_get(const httpdns_list_head_t *head, int index) {
    if (NULL == head || index < 0) {
        return NULL;
    }
    int i = 0;
    for (httpdns_list_node_t *cursor = httpdns_list_first_entry(head); cursor != head; cursor = cursor->next) {
        if (i == index) {
            return cursor->data;
        }
        i++;
    }
    return NULL;
}


size_t httpdns_list_size(const httpdns_list_head_t *head) {
    if (NULL == head) {
        return 0;
    }
    size_t size = 0;
    for (httpdns_list_node_t *cursor = httpdns_list_first_entry(head); cursor != head; cursor = cursor->next) {
        size++;
    }
    return size;
}


void httpdns_list_free(httpdns_list_head_t *head, httpdns_data_free_func_t free_func) {
    if (NULL == head) {
        return;
    }
    httpdns_list_node_t *cursor, *cursor_next;
    for (cursor = httpdns_list_first_entry(head), cursor_next = cursor->next;
         cursor != head; cursor = cursor_next, cursor_next = cursor_next->next) {
        httpdns_list_del_init(cursor);
        if (NULL != free_func) {
            free_func(cursor->data);
        }
        free(cursor);
    }
}

void httpdns_list_shuffle(httpdns_list_head_t *head) {
    if (NULL == head || httpdns_list_size(head) <= 1) {
        return;
    }
    size_t list_size = httpdns_list_size(head);
    for (int outer_loop_i = 0; outer_loop_i < 2 * list_size; outer_loop_i++) {
        int swap_index = rand() % (int) list_size;
        int inner_loop_i = 0;
        for (httpdns_list_node_t *cursor = httpdns_list_first_entry(head); cursor != head; cursor = cursor->next) {
            if (swap_index == inner_loop_i) {
                httpdns_list_del_init(cursor);
                httpdns_list_insert_tail(cursor, head);
                break;
            }
            inner_loop_i++;
        }
    }
}

bool httpdns_list_contain(const httpdns_list_head_t *head, const void *data, httpdns_data_cmp_func_t cmp_func) {
    if (NULL == head || NULL == data || NULL == cmp_func) {
        return false;
    }
    for (httpdns_list_node_t *cursor = httpdns_list_first_entry(head); cursor != head; cursor = cursor->next) {
        if (cmp_func(cursor->data, data) == 0) {
            return true;
        }
    }
    return false;
}

void *httpdns_list_min(const httpdns_list_head_t *head, httpdns_data_cmp_func_t cmp_func) {
    if (NULL == head || NULL == cmp_func) {
        return NULL;
    }
    void *min_data = NULL;
    for (httpdns_list_node_t *cursor = httpdns_list_first_entry(head); cursor != head; cursor = cursor->next) {
        if (NULL == min_data || cmp_func(cursor->data, min_data) < 0) {
            min_data = cursor->data;
        }
    }
    return min_data;
}

void *httpdns_list_max(const httpdns_list_head_t *head, httpdns_data_cmp_func_t cmp_func) {
    if (NULL == head || NULL == cmp_func) {
        return NULL;
    }
    void *max_data = NULL;
    for (httpdns_list_node_t *cursor = httpdns_list_first_entry(head); cursor != head; cursor = cursor->next) {
        if (NULL == max_data || cmp_func(cursor->data, max_data) > 0) {
            max_data = cursor->data;
        }
    }
    return max_data;
}

// 插入排序
void httpdns_list_sort(httpdns_list_head_t *head, httpdns_data_cmp_func_t cmp_func) {
    if (NULL == head || NULL == cmp_func) {
        return;
    }
    httpdns_list_node_t *cursor, *cursor_next;
    for (cursor = httpdns_list_first_entry(head), cursor_next = cursor->next;
         cursor != head; cursor = cursor_next, cursor_next = cursor_next->next) {
        // 寻找前驱
        httpdns_list_node_t *previous = cursor->prev;
        while (previous != head) {
            if (cmp_func(previous->data, cursor->data) <= 0) {
                break;
            }
            previous = previous->prev;
        }
        // 插入到前驱
        if (cursor != previous->next) {
            httpdns_list_del_init(cursor);
            httpdns_list_insert_head(cursor, previous);
        }
    }
}

httpdns_sds_t httpdns_list_to_string(const httpdns_list_head_t *head, httpdns_data_to_string_func_t to_string_func) {
    if (NULL == head) {
        return httpdns_sds_new("[]");
    }
    httpdns_sds_t dst_str = httpdns_sds_new("[");
    for (httpdns_list_node_t *cursor = httpdns_list_first_entry(head); cursor != head; cursor = cursor->next) {
        if (cursor->prev != head) {
            httpdns_sds_cat_easily(dst_str, ",");
        }
        if (NULL == to_string_func) {
            httpdns_sds_cat_easily(dst_str, cursor->data);
        } else {
            httpdns_sds_t node_data_str = to_string_func(cursor->data);
            httpdns_sds_cat_easily(dst_str, node_data_str);
            httpdns_sds_free(node_data_str);
        }
    }
    httpdns_sds_cat_easily(dst_str, "]");
    return dst_str;
}


void *httpdns_list_search(const httpdns_list_head_t *head, const void *target, httpdns_data_search_func_t search_func) {
    if (NULL == head || NULL == target || NULL == search_func) {
        return NULL;
    }
    for (httpdns_list_node_t *cursor = httpdns_list_first_entry(head); cursor != head; cursor = cursor->next) {
        if (search_func(cursor->data, target)) {
            return cursor->data;
        }
    }
    return NULL;
}

bool inline httpdns_list_is_end_node(const httpdns_list_node_t *node, const httpdns_list_head_t *head) {
    if (NULL == node || NULL == head) {
        return false;
    }
    return node->next == head;
}



