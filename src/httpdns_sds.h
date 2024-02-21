//
// Created by caogaoshuai on 2024/1/9.
// 参考linux sds.h
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SDS_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SDS_H

#ifdef __cplusplus
extern "C"
{
#endif

#define HTTPDNS_SDS_MAX_PREALLOC (1024*1024)

#include <sys/types.h>
#include <stdarg.h>
#include <string.h>


#define httpdns_string_is_not_blank(str) \
    (NULL != str && strlen(str) >0)

#define httpdns_string_is_blank(str) \
    ((NULL == str || strlen(str) <=0))

#define httpdns_sds_cat_easily(dst_str, slice) \
    dst_str = httpdns_sds_cat(dst_str, slice)


#define httpdns_sds_cat_int(dst_str, int_val) \
    {                                 \
        char tmp_buffer[26];                  \
        sprintf(tmp_buffer, "%d", int_val); \
        httpdns_sds_cat_easily(dst_str, tmp_buffer); \
    }

#define httpdns_sds_cat_char_easily(dst_str, char_val) \
     dst_str = httpdns_sds_cat_char(dst_str, char_val)

//#define SDS_CAT_CHAR(dst_str, ch_val) \
//    {                                 \
//        char tmp_buffer[2];           \
//        tmp_buffer[0]=ch_val;         \
//        tmp_buffer[1]='\0';         \
//        httpdns_sds_cat_easily(dst_str, tmp_buffer); \
//    }

#define HTTPDNS_MICRO_STRINGIFY(x) #x
#define HTTPDNS_MICRO_TO_STRING(x) HTTPDNS_MICRO_STRINGIFY(x)

#ifdef WIN32
#define inline __inline
#endif

typedef char *httpdns_sds_t;

typedef struct httpdns_sds_header_t {
    unsigned int len;
    unsigned int free;
    char buf[];
} httpdns_sds_header_t;

size_t httpdns_sds_len(const httpdns_sds_t s);

size_t httpdns_sds_avail(const httpdns_sds_t s);

httpdns_sds_t httpdns_sds_new_len(const void *init, size_t initlen);

httpdns_sds_t httpdns_sds_new_empty(size_t preAlloclen);

httpdns_sds_t httpdns_sds_new(const char *init);

httpdns_sds_t httpdns_sds_empty(void);

httpdns_sds_t httpdns_sds_dup(const httpdns_sds_t s);

void httpdns_sds_free(httpdns_sds_t s);

httpdns_sds_t httpdns_sds_grow_zero(httpdns_sds_t s, size_t len);

httpdns_sds_t httpdns_sds_cat_len(httpdns_sds_t s, const void *t, size_t len);

httpdns_sds_t httpdns_sds_cat(httpdns_sds_t s, const char *t);

httpdns_sds_t httpdns_sds_cat_char(httpdns_sds_t s, char c);

httpdns_sds_t httpdns_sds_cat_sds(httpdns_sds_t s, const httpdns_sds_t t);

httpdns_sds_t httpdns_sds_cpy_len(httpdns_sds_t s, const char *t, size_t len);

httpdns_sds_t httpdns_sds_cpy(httpdns_sds_t s, const char *t);

httpdns_sds_t httpdns_sds_cat_vprintf(httpdns_sds_t s, const char *fmt, va_list ap);

#ifdef __GNUC__

httpdns_sds_t httpdns_sds_cat_printf(httpdns_sds_t s, const char *fmt, ...)
__attribute__((format(printf, 2, 3)));

#else
httpdns_sds_t httpdns_sds_cat_printf(httpdns_sds_t s, const char *fmt, ...);
#endif


#ifdef __cplusplus
}
#endif

#endif
