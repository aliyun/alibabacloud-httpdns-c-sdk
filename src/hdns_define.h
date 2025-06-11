//
// Created by cagaoshuai on 2024/3/22.
//

#ifndef HDNS_C_SDK_HDNS_DEFINE_H
#define HDNS_C_SDK_HDNS_DEFINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <apr_portable.h>
#include <apr_time.h>
#include <apr_strings.h>
#include <apr_pools.h>
#include <apr_tables.h>
#include <apr_file_io.h>
#include <apr.h>
#include <apr_hash.h>

#include <curl/curl.h>


#ifdef __cplusplus
# define HDNS_CPP_START extern "C" {
# define HDNS_CPP_END }
#else
# define HDNS_CPP_START
# define HDNS_CPP_END
#endif

typedef enum {
    HDNS_HTTP_GET,
    HDNS_HTTP_HEAD,
    HDNS_HTTP_PUT,
    HDNS_HTTP_POST,
    HDNS_HTTP_DELETE
} http_method_e;

typedef enum {
    HDNS_STATE_INIT = 0,
    HDNS_STATE_START,
    HDNS_STATE_RUNNING,
    HDNS_STATE_STOPPING,
    HDNS_STATE_DESTROYED
} hdns_state_e;



typedef enum {
    HDNS_OK = 0,
    HDNS_FAILED_VERIFICATION = -999,
    HDNS_RESOLVE_FAIL = -998,
    HDNS_SCHEDULE_FAIL = -997,
    HDNS_OUT_MEMORY = -996,
    HDNS_FAILED_CONNECT = -995,
    HDNS_INTERNAL_ERROR = -994,
    HDNS_INVALID_ARGUMENT = -993,
    HDNS_CONNECTION_FAILED = -992,
    HDNS_NAME_LOOKUP_ERROR = -991,
    HDNS_WRITE_BODY_ERROR = -990,
    HDNS_READ_BODY_ERROR = -989,
    HDNS_OPEN_FILE_ERROR = -988,
    HDNS_ERROR = -1,
} hdns_err_code_e;


typedef enum {
    HDNS_QUERY_AUTO = 0,
    HDNS_QUERY_IPV4 = 0x01,
    HDNS_QUERY_IPV6 = 0x02,
    HDNS_QUERY_BOTH = 0x03,
} hdns_query_type_t;

// APR方法别名
typedef apr_hash_t hdns_hash_t;
typedef apr_pool_t hdns_pool_t;
typedef apr_table_t hdns_table_t;
typedef apr_table_entry_t hdns_table_entry_t;
typedef apr_array_header_t hdns_array_header_t;

#define hdns_table_elts(t) apr_table_elts(t)
#define hdns_is_empty_table(t) apr_is_empty_table(t)
#define hdns_table_make(p, n) apr_table_make(p, n)
#define hdns_table_add_int(t, key, value) do {       \
        char value_str[64];                             \
        apr_snprintf(value_str, sizeof(value_str), "%d", value);\
        apr_table_add(t, key, value_str);               \
    } while(0)

#define hdns_table_add_int64(t, key, value) do {       \
        char value_str[64];                             \
        apr_snprintf(value_str, sizeof(value_str), "%" APR_INT64_T_FMT, value);\
        apr_table_add(t, key, value_str);               \
    } while(0)

#define hdns_table_set_int64(t, key, value) do {       \
        char value_str[64];                             \
        apr_snprintf(value_str, sizeof(value_str), "%" APR_INT64_T_FMT, value);\
        apr_table_set(t, key, value_str);               \
    } while(0)

#define hdns_pool_create(n, p) apr_pool_create(n, p)
#define hdns_pool_destroy(p) apr_pool_destroy(p)
#define hdns_palloc(p, s) apr_palloc(p, s)
#define hdns_pcalloc(p, s) apr_pcalloc(p, s)

// HTTPDNS定义宏
#define hdns_to_long(v)   (long)(v)
#define hdns_to_int(v)   (int)(v)
#define hdns_to_char(v)   (char)(v)
#define hdns_to_void_p(v) (void*)(v)
#define hdns_unused_var(v) ((void)(v))
#define hdns_str_is_blank(str) \
    ((NULL == str || strlen(str) <=0))

#define hdns_str_is_not_blank(str) \
    (NULL != str && strlen(str) >0)

#define hdns_pool_new(p) \
     hdns_pool_t *p = NULL; \
     hdns_pool_create(&p, NULL)

#define hdns_pool_new_with_pp(p, pp) \
     hdns_pool_t *p = NULL; \
     hdns_pool_create(&p, pp)

// 字符串资源长度约束
#define HDNS_INIT_WINSOCK 1
#define HDNS_MD5_STRING_LEN 32
#define HDNS_SID_STRING_LEN 12
#define HDNS_MAX_URI_LEN 2048
#define HDNS_MAX_HEADER_LEN 8192
#define HDNS_MAX_QUERY_ARG_LEN 1024
#define HDNS_MAX_SDNS_EXTRA_LEN 1024
#define HDNS_MAX_GMT_TIME_LEN 128
#define HDNS_MAX_SHORT_TIME_LEN 10

#define HDNS_MAX_INT64_STRING_LEN 64
#define HDNS_MAX_UINT64_STRING_LEN 64
#define HDNS_IP_ADDRESS_STRING_LENGTH (40)

#define HDNS_REQUEST_STACK_SIZE 32

#define HDNS_MULTI_RESOLVE_SIZE 5
#define HDNS_MAX_DOMAIN_LENGTH  255

#define HDNS_HTTP_PREFIX    "http://"
#define HDNS_HTTPS_PREFIX   "https://"
#define HDNS_MAX_TIMEOUT_MS  3000
#define HDNS_MAX_CONNECT_TIMEOUT_MS  2500
#define HDNS_SCHEDULER_REFRESH_TIMEOUT_MS 2000
#define HDNS_MIN_TIMEOUT_MS  50
#define HDNS_SSL_CA_HOST     "resolvers.httpdns.aliyuncs.com"
#define HDNS_CERT_PEM_NAME   "Cert:"
#define HDNS_HTTP_STATUS_OK  200

#define hdns_abs(value)       (((value) >= 0) ? (value) : - (value))
#define hdns_max(val1, val2)  (((val1) < (val2)) ? (val2) : (val1))
#define hdns_min(val1, val2)  (((val1) > (val2)) ? (val2) : (val1))

#define LF     (char) 10
#define CR     (char) 13
#define CRLF   "\x0d\x0a"

#define HDNS_PLATFORM   "linux"
#define HDNS_VERSION    "2.2.5"
#define HDNS_VER        "alibabacloud-httpdns-c-sdk-" HDNS_VERSION

#define HDNS_FALSE     0
#define HDNS_TRUE      1

# ifndef WIN32
# ifndef __LONG_LONG_MAX__
# define __LONG_LONG_MAX__ 9223372036854775807LL
# endif
# ifndef LLONG_MIN
#  define LLONG_MIN (-LLONG_MAX-1)
# endif
# ifndef LLONG_MAX
#  define LLONG_MAX __LONG_LONG_MAX__
# endif
# ifndef ULLONG_MAX
#  define ULLONG_MAX (LLONG_MAX * 2ULL + 1)
# endif
# endif


#endif
