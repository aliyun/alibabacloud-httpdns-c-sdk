#include "hdns_string.h"
#include <apr_strings.h>

typedef int (*hdns_is_char_fn_t)(char c);

static void hdns_strip_str_func(hdns_string_t *str, hdns_is_char_fn_t func);

char *hdns_pstrdup(hdns_pool_t *p, const hdns_string_t *s) {
    return apr_pstrndup(p, s->data, s->len);
}

static void hdns_strip_str_func(hdns_string_t *str, hdns_is_char_fn_t func) {
    char *data = str->data;
    size_t len = str->len;
    int offset = 0;

    if (len == 0) return;

    while (len > 0 && func(data[len - 1])) {
        --len;
    }

    for (; offset < len && func(data[offset]); ++offset) {
        // empty;
    }

    str->data = data + offset;
    str->len = len - offset;
}

void hdns_unquote_str(hdns_string_t *str) {
    hdns_strip_str_func(str, hdns_is_quote);
}

void hdns_strip_space(hdns_string_t *str) {
    hdns_strip_str_func(str, hdns_is_space);
}

void hdns_trip_space_and_cntrl(hdns_string_t *str) {
    hdns_strip_str_func(str, hdns_is_space_or_cntrl);
}

int hdns_ends_with(const hdns_string_t *str, const hdns_string_t *suffix) {
    if (!str || !suffix) {
        return 0;
    }

    return (str->len >= suffix->len) && strncmp(str->data + str->len - suffix->len, suffix->data, suffix->len) == 0;
}

bool hdns_str_search(const char *s1, const char *s2) {
    return strcmp(s1, s2) == 0;
}


bool hdns_str_start_with(const char *str, const char *prefix) {
    apr_size_t prefix_len = strlen(prefix);
    apr_size_t str_len = strlen(str);

    if (str_len < prefix_len) {
        return FALSE;
    }
    return strncmp(str, prefix, prefix_len) == 0;
}