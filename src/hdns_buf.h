#ifndef HDNS_C_SDK_HDNS_BUF_H
#define HDNS_C_SDK_HDNS_BUF_H

#include "hdns_define.h"
#include "hdns_list.h"

HDNS_CPP_START

typedef struct {
    uint8_t *pos;
    uint8_t *last;
    uint8_t *start;
    uint8_t *end;
} hdns_buf_t;


hdns_buf_t *hdns_create_buf(hdns_pool_t *p, size_t size);

#define hdns_buf_size(b) (b->last - b->pos)

hdns_buf_t *hdns_buf_pack(hdns_pool_t *p, const void *data, size_t size);

char *hdns_buf_list_content(hdns_pool_t *p, hdns_list_head_t *list);

void hdns_buf_append_string(hdns_pool_t *p, hdns_buf_t *b, const char *str, size_t len);

HDNS_CPP_END

#endif

