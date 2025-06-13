//
// Created by caogaoshuai on 2024/1/10.
//
#include "hdns_list.h"

static APR_INLINE void hdns_list_insert(hdns_list_node_t *_new, hdns_list_node_t *prev, hdns_list_node_t *next);

static APR_INLINE void hdns_list_insert_head(hdns_list_node_t *_new, hdns_list_head_t *head);

static void hdns_list_init(hdns_list_head_t *head, hdns_pool_t *pool);

static void hdns_list_init(hdns_list_head_t *head, hdns_pool_t *pool) {
    head->next = head;
    head->prev = head;
    head->pool = pool;
    head->data = NULL;
}

static APR_INLINE void hdns_list_insert(hdns_list_node_t *_new,
                                        hdns_list_node_t *prev,
                                        hdns_list_node_t *next) {
    next->prev = _new;
    _new->next = next;
    _new->prev = prev;
    prev->next = _new;
}

static APR_INLINE void hdns_list_insert_head(hdns_list_node_t *_new, hdns_list_head_t *head) {
    hdns_list_insert(_new, head, head->next);
}

void hdns_list_insert_tail(hdns_list_node_t *_new, hdns_list_head_t *head) {
    hdns_list_insert(_new, head->prev, head);
}

int32_t hdns_list_add(hdns_list_head_t *head, const void *data, hdns_list_clone_fn_t clone) {
    if (NULL == head || NULL == data) {
        return HDNS_INVALID_ARGUMENT;
    }
    hdns_list_node_t *node = hdns_palloc(head->pool, sizeof(hdns_list_node_t));
    if (clone != NULL) {
        // apr在x86环境下的预编译库是stdcall模式，而vs默认约定是cdecl，就会导致源码和apr库的指针调用约定不一致
#if defined(_WIN32) && defined(_M_IX86)
        if (clone == (hdns_list_clone_fn_t)apr_pstrdup) {
            hdns_list_clone_stdcall_fn_t clone_with_call_convention = (hdns_list_clone_stdcall_fn_t)clone;
            node->data = clone_with_call_convention(head->pool, data);
        } else {
            node->data = clone(head->pool, data);
        }
#else
        node->data = clone(head->pool, data);
#endif
    } else {
        node->data = hdns_to_void_p(data);
    }
    hdns_list_insert_tail(node, head);
    return HDNS_OK;
}

hdns_list_node_t *hdns_list_first(const hdns_list_head_t *head) {
    return head->next;
}

void hdns_list_del(hdns_list_node_t *entry) {
    if (NULL == entry) {
        return;
    }
    hdns_list_node_t *next = entry->next;
    hdns_list_node_t *pre = entry->prev;
    pre->next = next;
    next->prev = pre;
}

int32_t hdns_list_rotate(hdns_list_head_t *head) {
    if (NULL == head || hdns_list_size(head) == 0) {
        return HDNS_INVALID_ARGUMENT;
    }
    if (hdns_list_size(head) == 1) {
        return HDNS_OK;
    }
    hdns_list_node_t *first_entry = hdns_list_first(head);
    hdns_list_del(first_entry);
    hdns_list_insert_tail(first_entry, head);
    return HDNS_OK;
}


void hdns_list_dup(hdns_list_head_t *dst_head,
                   const hdns_list_head_t *src_head,
                   hdns_list_clone_fn_t clone) {
    if (NULL == dst_head || NULL == src_head) {
        return;
    }
    for (hdns_list_node_t *cursor = hdns_list_first(src_head); cursor != src_head; cursor = cursor->next) {
        hdns_list_add(dst_head, cursor->data, clone);
    }
}

void hdns_list_filter(hdns_list_head_t *dst_head,
                      const hdns_list_head_t *src_head,
                      hdns_list_clone_fn_t clone,
                      hdns_list_filter_fn_t filter) {
    if (NULL == dst_head || NULL == src_head) {
        return;
    }
    for (hdns_list_node_t *cursor = hdns_list_first(src_head); cursor != src_head; cursor = cursor->next) {
        if (filter != NULL && !filter(cursor->data)) {
            continue;
        }
        hdns_list_add(dst_head, cursor->data, clone);
    }
}



void *hdns_list_get(const hdns_list_head_t *head, int index) {
    if (NULL == head || index < 0) {
        return NULL;
    }
    int i = 0;
    for (hdns_list_node_t *cursor = hdns_list_first(head); cursor != head; cursor = cursor->next) {
        if (i == index) {
            return cursor->data;
        }
        i++;
    }
    return NULL;
}


size_t hdns_list_size(const hdns_list_head_t *head) {
    if (NULL == head) {
        return 0;
    }
    size_t size = 0;
    for (hdns_list_node_t *cursor = hdns_list_first(head); cursor != head; cursor = cursor->next) {
        size++;
    }
    return size;
}

void hdns_list_shuffle(hdns_list_head_t *head) {
    if (NULL == head || hdns_list_size(head) <= 1) {
        return;
    }
    size_t list_size = hdns_list_size(head);
    for (int outer_loop_i = 0; outer_loop_i < 2 * list_size; outer_loop_i++) {
        int swap_index = rand() % (int) list_size;
        int inner_loop_i = 0;
        for (hdns_list_node_t *cursor = hdns_list_first(head); cursor != head; cursor = cursor->next) {
            if (swap_index == inner_loop_i) {
                hdns_list_del(cursor);
                hdns_list_insert_tail(cursor, head);
                break;
            }
            inner_loop_i++;
        }
    }
}

bool hdns_list_contain(const hdns_list_head_t *head, const void *data, hdns_list_cmp_pt cmp) {
    if (NULL == head || NULL == data || NULL == cmp) {
        return false;
    }
    for (hdns_list_node_t *cursor = hdns_list_first(head); cursor != head; cursor = cursor->next) {
        if (cmp(cursor->data, data) == 0) {
            return true;
        }
    }
    return false;
}

void *hdns_list_min(const hdns_list_head_t *head, hdns_list_cmp_pt cmp) {
    if (NULL == head || NULL == cmp) {
        return NULL;
    }
    void *min_data = NULL;
    for (hdns_list_node_t *cursor = hdns_list_first(head); cursor != head; cursor = cursor->next) {
        if (NULL == min_data || cmp(cursor->data, min_data) < 0) {
            min_data = cursor->data;
        }
    }
    return min_data;
}

void *hdns_list_max(const hdns_list_head_t *head, hdns_list_cmp_pt cmp) {
    if (NULL == head || NULL == cmp) {
        return NULL;
    }
    void *max_data = NULL;
    for (hdns_list_node_t *cursor = hdns_list_first(head); cursor != head; cursor = cursor->next) {
        if (NULL == max_data || cmp(cursor->data, max_data) > 0) {
            max_data = cursor->data;
        }
    }
    return max_data;
}

// 插入排序
void hdns_list_sort(hdns_list_head_t *head, hdns_list_cmp_pt cmp) {
    if (NULL == head || NULL == cmp) {
        return;
    }
    hdns_list_node_t *cursor, *cursor_next;
    for (cursor = hdns_list_first(head), cursor_next = cursor->next;
         cursor != head; cursor = cursor_next, cursor_next = cursor_next->next) {
        // 寻找前驱
        hdns_list_node_t *previous = cursor->prev;
        while (previous != head) {
            if (cmp(previous->data, cursor->data) <= 0) {
                break;
            }
            previous = previous->prev;
        }
        // 插入到前驱
        if (cursor != previous->next) {
            hdns_list_del(cursor);
            hdns_list_insert_head(cursor, previous);
        }
    }
}

void *hdns_list_search(const hdns_list_head_t *head, const void *target, hdns_list_search_pt search) {
    if (NULL == head || NULL == target || NULL == search) {
        return NULL;
    }
    for (hdns_list_node_t *cursor = hdns_list_first(head); cursor != head; cursor = cursor->next) {
        if (search(cursor->data, target)) {
            return cursor->data;
        }
    }
    return NULL;
}

bool hdns_list_is_end_node(const hdns_list_node_t *node, const hdns_list_head_t *head) {
    if (NULL == node || NULL == head) {
        return false;
    }
    return node->next == head;
}

hdns_list_head_t *hdns_list_new(hdns_pool_t *pool) {
    if (NULL == pool) {
        hdns_pool_create(&pool, NULL);
    }
    hdns_list_head_t *list = hdns_palloc(pool, sizeof(hdns_list_head_t));
    hdns_list_init(list, pool);
    return list;
}

void hdns_list_free(hdns_list_head_t *list) {
    if (list != NULL && list->pool != NULL) {
        hdns_pool_destroy(list->pool);
    }
}