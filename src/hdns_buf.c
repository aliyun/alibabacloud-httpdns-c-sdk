#include "hdns_buf.h"
#include "hdns_log.h"

hdns_buf_t *hdns_create_buf(hdns_pool_t *p, size_t size) {
    hdns_buf_t *b;

    b = hdns_palloc(p, sizeof(hdns_buf_t) + size);
    if (b == NULL) {
        return NULL;
    }

    b->pos = (uint8_t *) b + sizeof(hdns_buf_t);
    b->start = b->pos;
    b->last = b->start;
    b->end = b->last + size;

    return b;
}

hdns_buf_t *hdns_buf_pack(hdns_pool_t *p, const void *data, size_t size) {
    hdns_buf_t *b;

    b = hdns_palloc(p, sizeof(hdns_buf_t));
    if (b == NULL) {
        return NULL;
    }

    b->pos = (uint8_t *) data;
    b->start = b->pos;
    b->last = b->start + size;
    b->end = b->last;

    return b;
}

int64_t hdns_buf_list_len(hdns_list_head_t *list) {
    int64_t len = 0;

    hdns_list_for_each_entry(buf_cursor, list) {
        len += hdns_buf_size(((hdns_buf_t *) buf_cursor->data));
    }

    return len;
}

char *hdns_buf_list_content(hdns_pool_t *p, hdns_list_head_t *list) {
    int64_t body_len;
    char *buf;
    int64_t pos = 0;
    int64_t size;
    hdns_buf_t *content;

    body_len = hdns_buf_list_len(list);
    buf = hdns_pcalloc(p, (size_t) (body_len + 1));
    buf[body_len] = '\0';
    hdns_list_for_each_entry(buf_cursor, list) {
        content = buf_cursor->data;
        size = hdns_buf_size(content);
        memcpy(buf + pos, content->pos, (size_t) (size));
        pos += size;
    }
    return buf;
}


void hdns_buf_append_string(hdns_pool_t *p, hdns_buf_t *b, const char *str, size_t len) {
    size_t size;
    size_t nsize;
    size_t remain;
    char *buf;

    if (len <= 0) return;

    remain = b->end - b->last;

    if (remain > len + 128) {
        memcpy(b->last, str, len);
        b->last += len;
    } else {
        size = hdns_buf_size(b);
        nsize = (size + len) * 2;
        buf = hdns_palloc(p, nsize);
        memcpy(buf, b->pos, size);
        memcpy(buf + size, str, len);
        b->start = (uint8_t *) buf;
        b->end = (uint8_t *) buf + nsize;
        b->pos = (uint8_t *) buf;
        b->last = (uint8_t *) buf + size + len;
    }
}
