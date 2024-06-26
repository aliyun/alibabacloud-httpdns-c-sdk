#ifndef HDNS_C_SDK_HDNS_STRING_H
#define HDNS_C_SDK_HDNS_STRING_H

#include "hdns_define.h"

HDNS_CPP_START

typedef struct {
    size_t len;
    char *data;
} hdns_string_t;

#define hdns_string(str)     { sizeof(str) - 1, (char *) str }
#define hdns_null_string     { 0, NULL }
#define hdns_str_set(str, text)                                  \
    (str)->len = strlen(text); (str)->data = (char *) text
#define hdns_str_null(str)   (str)->len = 0; (str)->data = NULL

#define hdns_tolower(c)      (char) ((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)
#define hdns_toupper(c)      (char) ((c >= 'a' && c <= 'z') ? (c & ~0x20) : c)

static APR_INLINE int hdns_string_is_empty(const hdns_string_t *str) {
    if (NULL == str || str->len == 0 || NULL == str->data || 0 == strcmp(str->data, "")) {
        return HDNS_TRUE;
    } else {
        return HDNS_FALSE;
    }
}

static APR_INLINE void hdns_string_tolower(hdns_string_t *str) {
    int i = 0;
    while (i < str->len) {
        str->data[i] = hdns_tolower(str->data[i]);
        ++i;
    }
}

static APR_INLINE char *hdns_strlchr(char *p, char *last, char c) {
    while (p < last) {
        if (*p == c) {
            return p;
        }
        p++;
    }
    return NULL;
}

static APR_INLINE int hdns_is_quote(char c) {
    return c == '\"';
}

static APR_INLINE int hdns_is_space(char c) {
    return ((c == ' ') || (c == '\t'));
}

static APR_INLINE int hdns_is_space_or_cntrl(char c) {
    return c <= ' ';
}

static APR_INLINE int hdns_is_null_string(hdns_string_t *str) {
    if (str == NULL || str->data == NULL || str->len == 0) {
        return HDNS_TRUE;
    }
    return HDNS_FALSE;
}

void hdns_strip_space(hdns_string_t *str);

void hdns_trip_space_and_cntrl(hdns_string_t *str);

void hdns_unquote_str(hdns_string_t *str);

char *hdns_pstrdup(hdns_pool_t *p, const hdns_string_t *s);

int hdns_ends_with(const hdns_string_t *str, const hdns_string_t *suffix);

bool hdns_str_search(const char *s1, const char *s2);

bool hdns_str_start_with(const char * str, const char* prefix);

HDNS_CPP_END

#endif
