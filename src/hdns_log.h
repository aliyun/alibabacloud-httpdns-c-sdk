#ifndef HDNS_C_SDK_HDNS_LOG_H
#define HDNS_C_SDK_HDNS_LOG_H

#include "hdns_define.h"

HDNS_CPP_START

typedef void (*hdns_log_print_pt)(const char *message, int len);

typedef void (*hdns_log_format_pt)(int level,
                                   const char *file,
                                   int line,
                                   const char *function,
                                   const char *fmt, ...)
        __attribute__ ((__format__ (__printf__, 5, 6)));

typedef enum {
    HDNS_LOG_OFF = -1,
    HDNS_LOG_FATAL,
    HDNS_LOG_ERROR,
    HDNS_LOG_WARN,
    HDNS_LOG_INFO,
    HDNS_LOG_DEBUG,
    HDNS_LOG_TRACE
} hdns_log_level_e;

#ifdef WIN32
#define hdns_log_fatal(format, ...) if(hdns_log_level>=HDNS_LOG_FATAL) \
        hdns_log_format(HDNS_LOG_FATAL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define hdns_log_error(format, ...) if(hdns_log_level>=HDNS_LOG_ERROR) \
        hdns_log_format(HDNS_LOG_ERROR, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define hdns_log_warn(format, ...) if(hdns_log_level>=HDNS_LOG_WARN)   \
        hdns_log_format(HDNS_LOG_WARN, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define hdns_log_info(format, ...) if(hdns_log_level>=HDNS_LOG_INFO)   \
        hdns_log_format(HDNS_LOG_INFO, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define hdns_log_debug(format, ...) if(hdns_log_level>=HDNS_LOG_DEBUG) \
        hdns_log_format(HDNS_LOG_DEBUG, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define hdns_log_trace(format, ...) if(hdns_log_level>=HDNS_LOG_TRACE) \
        hdns_log_format(HDNS_LOG_TRACE, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#else
#define hdns_log_fatal(format, args...) if(hdns_log_level>=HDNS_LOG_FATAL) \
        hdns_log_format(HDNS_LOG_FATAL, __FILE__, __LINE__, __FUNCTION__, format, ## args)
#define hdns_log_error(format, args...) if(hdns_log_level>=HDNS_LOG_ERROR) \
        hdns_log_format(HDNS_LOG_ERROR, __FILE__, __LINE__, __FUNCTION__, format, ## args)
#define hdns_log_warn(format, args...) if(hdns_log_level>=HDNS_LOG_WARN)   \
        hdns_log_format(HDNS_LOG_WARN, __FILE__, __LINE__, __FUNCTION__, format, ## args)
#define hdns_log_info(format, args...) if(hdns_log_level>=HDNS_LOG_INFO)   \
        hdns_log_format(HDNS_LOG_INFO, __FILE__, __LINE__, __FUNCTION__, format, ## args)
#define hdns_log_debug(format, args...) if(hdns_log_level>=HDNS_LOG_DEBUG) \
        hdns_log_format(HDNS_LOG_DEBUG, __FILE__, __LINE__, __FUNCTION__, format, ## args)
#define hdns_log_trace(format, args...) if(hdns_log_level>=HDNS_LOG_TRACE) \
        hdns_log_format(HDNS_LOG_TRACE, __FILE__, __LINE__, __FUNCTION__, format, ## args)
#endif

void hdns_log_create(hdns_pool_t* pool);

void hdns_log_cleanup();

void hdns_log_set_print(hdns_log_print_pt p);

void hdns_log_set_format(hdns_log_format_pt p);

void hdns_log_set_level(hdns_log_level_e level);

void hdns_log_set_output(apr_file_t *output);

void hdns_log_print_default(const char *message, int len);

void hdns_log_format_default(int level,
                             const char *file,
                             int line,
                             const char *function,
                             const char *fmt, ...)
__attribute__ ((__format__ (__printf__, 5, 6)));

extern hdns_log_level_e hdns_log_level;
extern hdns_log_format_pt hdns_log_format;
extern apr_file_t *hdns_stdout_file;

HDNS_CPP_END

#endif
