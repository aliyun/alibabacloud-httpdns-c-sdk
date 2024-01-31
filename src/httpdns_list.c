//
// Created by cagaoshuai on 2024/1/10.
//
#include "httpdns_list.h"
#include "httpdns_string.h"
#include <stdio.h>

void httpdns_list_init(struct list_head *ips) {
    if (NULL == ips) {
        return;
    }
    INIT_LIST_HEAD(ips);
}

int32_t httpdns_list_add(struct list_head *head, const void *data, data_clone_function_ptr_t clone_func) {
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
        list_add_tail(&node->list, head);
        return HTTPDNS_SUCCESS;
    }
    return HTTPDNS_MEMORY_ALLOCATION_ERROR;
}

int32_t httpdns_list_rotate(struct list_head *head) {
    if (NULL == head || httpdns_list_size(head) == 0) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (httpdns_list_size(head) == 1) {
        return HTTPDNS_SUCCESS;
    }
    httpdns_list_node_t *first_entry = list_first_entry(head, httpdns_list_node_t, list);
    list_del_init(&first_entry->list);
    list_add_tail(&first_entry->list, head);
    return HTTPDNS_SUCCESS;
}


struct list_head *
httpdns_list_dup(struct list_head *dst_head, struct list_head *src_head, data_clone_function_ptr_t clone_func) {
    if (NULL == dst_head || NULL == src_head || NULL == clone_func) {
        return NULL;
    }
    httpdns_list_node_t *cursor;
    httpdns_list_init(dst_head);
    list_for_each_entry(cursor, src_head, list) {
        httpdns_list_add(dst_head, cursor->data, clone_func);
    }
    return dst_head;
}

void *httpdns_list_get(struct list_head *head, int index) {
    if (NULL == head || index < 0) {
        return NULL;
    }
    int i = 0;
    httpdns_list_node_t *cursor;
    list_for_each_entry(cursor, head, list) {
        if (i == index) {
            return cursor->data;
        }
        i++;
    }
    return NULL;
}


size_t httpdns_list_size(struct list_head *head) {
    if (NULL == head) {
        return 0;
    }
    httpdns_list_node_t *cursor;
    size_t size = 0;
    list_for_each_entry(cursor, head, list) {
        size++;
    }
    return size;
}

void httpdns_list_free(struct list_head *head, data_free_function_ptr_t free_func) {
    if (NULL == head) {
        return;
    }
    httpdns_list_node_t *cursor, *temp_node;
    list_for_each_entry_safe(cursor, temp_node, head, list) {
        list_del(&cursor->list);
        if (NULL != free_func) {
            free_func(cursor->data);
        }
        free(cursor);
    }
}

void httpdns_list_shuffle(struct list_head *head) {
    if (NULL == head || httpdns_list_size(head) <= 1) {
        return;
    }
    size_t list_size = httpdns_list_size(head);
    for (int outer_loop_i = 0; outer_loop_i < 2 * list_size; outer_loop_i++) {
        int swap_index = rand() % (int) list_size;
        httpdns_list_node_t *cursor;
        int inner_loop_i = 0;
        list_for_each_entry(cursor, head, list) {
            if (swap_index == inner_loop_i) {
                list_del_init(&cursor->list);
                list_add_tail(&cursor->list, head);
            }
            inner_loop_i++;
        }
    }
}

bool httpdns_list_contain(struct list_head *head, const void *data, data_cmp_function_ptr_t cmp_func) {
    if (NULL == head || NULL == data || NULL == cmp_func) {
        return false;
    }
    httpdns_list_node_t *cursor;
    list_for_each_entry(cursor, head, list) {
        if (cmp_func(cursor->data, data) == 0) {
            return true;
        }
    }
    return false;
}

void *httpdns_list_min(struct list_head *head, data_cmp_function_ptr_t cmp_func) {
    if (NULL == head || NULL == cmp_func) {
        return NULL;
    }
    httpdns_list_node_t *cursor;
    void *min_data = NULL;
    list_for_each_entry(cursor, head, list) {
        if (NULL == min_data || cmp_func(cursor->data, min_data) < 0) {
            min_data = cursor->data;
        }
    }
    return min_data;
}

void *httpdns_list_max(struct list_head *head, data_cmp_function_ptr_t cmp_func) {
    if (NULL == head || NULL == cmp_func) {
        return NULL;
    }
    httpdns_list_node_t *cursor;
    void *max_data = NULL;
    list_for_each_entry(cursor, head, list) {
        if (NULL == max_data || cmp_func(cursor->data, max_data) > 0) {
            max_data = cursor->data;
        }
    }
    return max_data;
}

void httpdns_list_sort(struct list_head *head, data_cmp_function_ptr_t cmp_func) {
    if (NULL == head || NULL == cmp_func) {
        return;
    }
    struct list_head *current, *previous, *next_tmp;
    httpdns_list_node_t *current_data;
    list_for_each_safe(current, next_tmp, head) {
        current_data = list_entry(current, httpdns_list_node_t, list);
        previous = current->prev;
        while (previous != head) {
            httpdns_list_node_t *previous_data = list_entry(previous, httpdns_list_node_t, list);
            if (cmp_func(previous_data->data, current_data->data) <= 0) {
                break;
            }
            previous = previous->prev;
        }
        if (current != previous->next) {
            list_move(current, previous);
        }
    }
}

sds httpdns_list_to_string(struct list_head *head, data_to_string_function_ptr_t to_string_func) {
    if (NULL == head) {
        return sdsnew("[]");
    }
    sds dst_str = sdsnew("[");
    httpdns_list_node_t *cursor, *temp_node;
    list_for_each_entry_safe(cursor, temp_node, head, list) {
        if (cursor->list.prev != head) {
            SDS_CAT(dst_str, ",");
        }
        if (NULL == to_string_func) {
            SDS_CAT(dst_str, cursor->data);
        } else {
            sds node_data_str = to_string_func(cursor->data);
            SDS_CAT(dst_str, node_data_str);
            sdsfree(node_data_str);
        }
    }
    SDS_CAT(dst_str, "]");
    return dst_str;
}


void *httpdns_list_search(struct list_head *head, const void *target, data_search_function_ptr_t search_func) {
    if (NULL == head || NULL == target || NULL == search_func) {
        return NULL;
    }
    httpdns_list_node_t *cursor;
    list_for_each_entry(cursor, head, list) {
        if (search_func(cursor->data, target)) {
            return cursor->data;
        }
    }
    return NULL;
}




