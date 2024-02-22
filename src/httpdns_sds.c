//
// Created by caogaoshuai on 2024/1/9.
// 参考linux sds.c
//

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "httpdns_sds.h"

size_t httpdns_sds_len(const httpdns_sds_t s) {
    httpdns_sds_header_t *sh = (httpdns_sds_header_t *) (s - (sizeof(httpdns_sds_header_t)));
    return sh->len;
}

size_t httpdns_sds_avail(const httpdns_sds_t s) {
    httpdns_sds_header_t *sh = (httpdns_sds_header_t *) (s - (sizeof(httpdns_sds_header_t)));
    return sh->free;
}

/* Create a new httpdns_sds_t string with the content specified by the 'init' pointer
 * and 'initlen'.
 * If NULL is used for 'init' the string is initialized with zero bytes.
 *
 * The string is always null-termined (all the httpdns_sds_t strings are, always) so
 * even if you create an httpdns_sds_t string with:
 *
 * mystring = httpdns_sds_new_len("abc",3);
 *
 * You can print the string with printf() as there is an implicit \0 at the
 * end of the string. However the string is binary safe and can contain
 * \0 characters in the middle, as the length is stored in the httpdns_sds_t header. */
httpdns_sds_t httpdns_sds_new_len(const void *init, size_t initlen) {
    httpdns_sds_header_t *sh;

    if (init) {
        sh = malloc(sizeof(httpdns_sds_header_t) + initlen + 1);
    } else {
        sh = calloc(sizeof(httpdns_sds_header_t) + initlen + 1, 1);
    }
    if (sh == NULL) return NULL;
    sh->len = initlen;
    sh->free = 0;
    if (initlen && init)
        memcpy(sh->buf, init, initlen);
    sh->buf[initlen] = '\0';
    return (char *) sh->buf;
}


httpdns_sds_t httpdns_sds_new_empty(size_t preAlloclen) {
    httpdns_sds_header_t *sh;

    sh = malloc(sizeof(httpdns_sds_header_t) + preAlloclen + 1);
    if (sh == NULL) return NULL;
    sh->len = 0;
    sh->free = preAlloclen;
    sh->buf[0] = '\0';
    return (char *) sh->buf;
}


/* Create an empty (zero length) httpdns_sds_t string. Even in this case the string
 * always has an implicit null term. */
httpdns_sds_t httpdns_sds_empty(void) {
    return httpdns_sds_new_len("", 0);
}

/* Create a new httpdns_sds_t string starting from a null terminated C string. */
httpdns_sds_t httpdns_sds_new(const char *init) {
    size_t initlen = (init == NULL) ? 0 : strlen(init);
    return httpdns_sds_new_len(init, initlen);
}

/* Duplicate an httpdns_sds_t string. */
httpdns_sds_t httpdns_sds_dup(const httpdns_sds_t s) {
    if (s == NULL) return NULL;
    return httpdns_sds_new_len(s, httpdns_sds_len(s));
}

/* Free an httpdns_sds_t string. No operation is performed if 's' is NULL. */
void httpdns_sds_free(httpdns_sds_t s) {
    if (s == NULL) return;
    free(s - sizeof(httpdns_sds_header_t));
}

/* Set the httpdns_sds_t string length to the length as obtained with strlen(), so
 * considering as content only up to the first null term character.
 *
 * This function is useful when the httpdns_sds_t string is hacked manually in some
 * way, like in the following example:
 *
 * s = httpdns_sds_new("foobar");
 * s[2] = '\0';
 * sdsupdatelen(s);
 * printf("%d\n", httpdns_sds_len(s));
 *
 * The output will be "2", but if we comment out the call to sdsupdatelen()
 * the output will be "6" as the string was modified but the logical length
 * remains 6 bytes. */
void sdsupdatelen(httpdns_sds_t s) {
    httpdns_sds_header_t *sh = (void *) (s - (sizeof(httpdns_sds_header_t)));
    int reallen = strlen(s);
    sh->free += (sh->len - reallen);
    sh->len = reallen;
}

/* Modify an httpdns_sds_t string in-place to make it empty (zero length).
 * However all the existing buffer is not discarded but set as free space
 * so that next append operations will not require allocations up to the
 * number of bytes previously available. */
void sdsclear(httpdns_sds_t s) {
    httpdns_sds_header_t *sh = (void *) (s - (sizeof(httpdns_sds_header_t)));
    sh->free += sh->len;
    sh->len = 0;
    sh->buf[0] = '\0';
}

/* Enlarge the free space at the end of the httpdns_sds_t string so that the caller
 * is sure that after calling this function can overwrite up to addlen
 * bytes after the end of the string, plus one more byte for nul term.
 *
 * Note: this does not change the *length* of the httpdns_sds_t string as returned
 * by httpdns_sds_len(), but only the free buffer space we have. */
httpdns_sds_t sdsMakeRoomFor(httpdns_sds_t s, size_t addlen) {
    httpdns_sds_header_t *sh, *newsh;
    size_t free = httpdns_sds_avail(s);
    size_t len, newlen;

    if (free >= addlen) return s;
    len = httpdns_sds_len(s);
    sh = (void *) (s - (sizeof(httpdns_sds_header_t)));
    newlen = (len + addlen);
    if (newlen < HTTPDNS_SDS_MAX_PREALLOC)
        newlen *= 2;
    else
        newlen += HTTPDNS_SDS_MAX_PREALLOC;
    newsh = realloc(sh, sizeof(httpdns_sds_header_t) + newlen + 1);
    if (newsh == NULL) return NULL;

    newsh->free = newlen - len;
    return newsh->buf;
}

/* Reallocate the httpdns_sds_t string so that it has no free space at the end. The
 * contained string remains not altered, but next concatenation operations
 * will require a reallocation.
 *
 * After the call, the passed httpdns_sds_t string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call. */
httpdns_sds_t sdsRemoveFreeSpace(httpdns_sds_t s) {
    httpdns_sds_header_t *sh;

    sh = (void *) (s - (sizeof(httpdns_sds_header_t)));
    sh = realloc(sh, sizeof(httpdns_sds_header_t) + sh->len + 1);
    sh->free = 0;
    return sh->buf;
}

/* Return the total size of the allocation of the specifed httpdns_sds_t string,
 * including:
 * 1) The httpdns_sds_t header before the pointer.
 * 2) The string.
 * 3) The free buffer at the end if any.
 * 4) The implicit null term.
 */
size_t sdsAllocSize(httpdns_sds_t s) {
    httpdns_sds_header_t *sh = (void *) (s - (sizeof(httpdns_sds_header_t)));

    return sizeof(*sh) + sh->len + sh->free + 1;
}

/* Increment the httpdns_sds_t length and decrements the left free space at the
 * end of the string according to 'incr'. Also set the null term
 * in the new end of the string.
 *
 * This function is used in order to fix the string length after the
 * user calls sdsMakeRoomFor(), writes something after the end of
 * the current string, and finally needs to set the new length.
 *
 * Note: it is possible to use a negative increment in order to
 * right-trim the string.
 *
 * Usage example:
 *
 * Using sdsIncrLen() and sdsMakeRoomFor() it is possible to mount the
 * following schema, to cat bytes coming from the kernel to the end of an
 * httpdns_sds_t string without copying into an intermediate buffer:
 *
 * oldlen = httpdns_sds_len(s);
 * s = sdsMakeRoomFor(s, BUFFER_SIZE);
 * nread = read(fd, s+oldlen, BUFFER_SIZE);
 * ... check for nread <= 0 and handle it ...
 * sdsIncrLen(s, nread);
 */
void sdsIncrLen(httpdns_sds_t s, int incr) {
    httpdns_sds_header_t *sh = (void *) (s - (sizeof(httpdns_sds_header_t)));

    if (incr >= 0)
        assert(sh->free >= (unsigned int) incr);
    else
        assert(sh->len >= (unsigned int) (-incr));
    sh->len += incr;
    sh->free -= incr;
    s[sh->len] = '\0';
}

/* Grow the httpdns_sds_t to have the specified length. Bytes that were not part of
 * the original length of the httpdns_sds_t will be set to zero.
 *
 * if the specified length is smaller than the current length, no operation
 * is performed. */
httpdns_sds_t httpdns_sds_grow_zero(httpdns_sds_t s, size_t len) {
    httpdns_sds_header_t *sh = (void *) (s - (sizeof(httpdns_sds_header_t)));
    size_t totlen, curlen = sh->len;

    if (len <= curlen) return s;
    s = sdsMakeRoomFor(s, len - curlen);
    if (s == NULL) return NULL;

    /* Make sure added region doesn't contain garbage */
    sh = (void *) (s - (sizeof(httpdns_sds_header_t)));
    memset(s + curlen, 0, (len - curlen + 1)); /* also set trailing \0 byte */
    totlen = sh->len + sh->free;
    sh->len = len;
    sh->free = totlen - sh->len;
    return s;
}

/* Append the specified binary-safe string pointed by 't' of 'len' bytes to the
 * end of the specified httpdns_sds_t string 's'.
 *
 * After the call, the passed httpdns_sds_t string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call. */
httpdns_sds_t httpdns_sds_cat_len(httpdns_sds_t s, const void *t, size_t len) {
    httpdns_sds_header_t *sh;
    size_t curlen = httpdns_sds_len(s);

    s = sdsMakeRoomFor(s, len);
    if (s == NULL) return NULL;
    sh = (void *) (s - (sizeof(httpdns_sds_header_t)));
    memcpy(s + curlen, t, len);
    sh->len = curlen + len;
    sh->free = sh->free - len;
    s[curlen + len] = '\0';
    return s;
}


httpdns_sds_t httpdns_sds_cat_char(httpdns_sds_t s, char c) {
    httpdns_sds_header_t *sh;
    size_t curlen = httpdns_sds_len(s);

    s = sdsMakeRoomFor(s, 1);
    if (s == NULL) return NULL;
    sh = (void *) (s - (sizeof(httpdns_sds_header_t)));
    s[curlen] = c;
    s[curlen + 1] = '\0';
    ++sh->len;
    --sh->free;
    return s;
}


/* Append the specified null termianted C string to the httpdns_sds_t string 's'.
 *
 * After the call, the passed httpdns_sds_t string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call. */
httpdns_sds_t httpdns_sds_cat(httpdns_sds_t s, const char *t) {
    if (s == NULL || t == NULL) {
        return s;
    }
    return httpdns_sds_cat_len(s, t, strlen(t));
}

/* Append the specified httpdns_sds_t 't' to the existing httpdns_sds_t 's'.
 *
 * After the call, the modified httpdns_sds_t string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call. */
httpdns_sds_t httpdns_sds_cat_sds(httpdns_sds_t s, const httpdns_sds_t t) {
    return httpdns_sds_cat_len(s, t, httpdns_sds_len(t));
}

/* Destructively modify the httpdns_sds_t string 's' to hold the specified binary
 * safe string pointed by 't' of length 'len' bytes. */
httpdns_sds_t httpdns_sds_cpy_len(httpdns_sds_t s, const char *t, size_t len) {
    httpdns_sds_header_t *sh = (void *) (s - (sizeof(httpdns_sds_header_t)));
    size_t totlen = sh->free + sh->len;

    if (totlen < len) {
        s = sdsMakeRoomFor(s, len - sh->len);
        if (s == NULL) return NULL;
        sh = (void *) (s - (sizeof(httpdns_sds_header_t)));
        totlen = sh->free + sh->len;
    }
    memcpy(s, t, len);
    s[len] = '\0';
    sh->len = len;
    sh->free = totlen - len;
    return s;
}

/* Like httpdns_sds_cpy_len() but 't' must be a null-termined string so that the length
 * of the string is obtained with strlen(). */
httpdns_sds_t httpdns_sds_cpy(httpdns_sds_t s, const char *t) {
    return httpdns_sds_cpy_len(s, t, strlen(t));
}



/* Like httpdns_sds_cat_printf() but gets va_list instead of being variadic. */
httpdns_sds_t httpdns_sds_cat_vprintf(httpdns_sds_t s, const char *fmt, va_list ap) {
    va_list cpy;
    char staticbuf[1024], *buf = staticbuf, *t;
    size_t buflen = strlen(fmt) * 2;

    /* We try to start using a static buffer for speed.
     * If not possible we revert to heap allocation. */
    if (buflen > sizeof(staticbuf)) {
        buf = malloc(buflen);
        if (buf == NULL) return NULL;
    } else {
        buflen = sizeof(staticbuf);
    }

    /* Try with buffers two times bigger every time we fail to
     * fit the string in the current buffer size. */
    while (1) {
        buf[buflen - 2] = '\0';
        va_copy(cpy, ap);
        vsnprintf(buf, buflen, fmt, cpy);
        va_end(cpy);
        if (buf[buflen - 2] != '\0') {
            if (buf != staticbuf) free(buf);
            buflen *= 2;
            buf = malloc(buflen);
            if (buf == NULL) return NULL;
            continue;
        }
        break;
    }

    /* Finally concat the obtained string to the SDS string and return it. */
    t = httpdns_sds_cat(s, buf);
    if (buf != staticbuf) free(buf);
    return t;
}

/* Append to the httpdns_sds_t string 's' a string obtained using printf-alike format
 * specifier.
 *
 * After the call, the modified httpdns_sds_t string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call.
 *
 * Example:
 *
 * s = httpdns_sds_new("Sum is: ");
 * s = httpdns_sds_cat_printf(s,"%d+%d = %d",a,b,a+b).
 *
 * Often you need to create a string from scratch with the printf-alike
 * format. When this is the need, just use httpdns_sds_empty() as the target string:
 *
 * s = httpdns_sds_cat_printf(httpdns_sds_empty(), "... your format ...", args);
 */
httpdns_sds_t httpdns_sds_cat_printf(httpdns_sds_t s, const char *fmt, ...) {
    va_list ap;
    char *t;
    va_start(ap, fmt);
    t = httpdns_sds_cat_vprintf(s, fmt, ap);
    va_end(ap);
    return t;
}


