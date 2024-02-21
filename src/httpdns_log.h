//
// Created by caogaoshuai on 2024/1/9.
// 参考log.c https://github.com/rxi/log.c
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_LOG_H
#define HTTPDNS_C_SDK_HTTPDNS_LOG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include "httpdns_env_configuration.h"

typedef struct {
    va_list ap;
    const char *fmt;
    const char *file;
    struct tm *time;
    void *udata;
    int line;
    const char *func;
    int level;
} httpdns_log_event_t;

typedef void (*httpdns_log_log_func_t)(httpdns_log_event_t *ev);

typedef void (*httpdns_log_lock_func_t)(bool lock, void *udata);

enum {
    HTTPDNS_LOG_TRACE, HTTPDNS_LOG_DEBUG, HTTPDNS_LOG_INFO, HTTPDNS_LOG_WARN, HTTPDNS_LOG_ERROR, HTTPDNS_LOG_FATAL
};

#define httpdns_log_trace(...) httpdns_log_log(HTTPDNS_LOG_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define httpdns_log_debug(...) httpdns_log_log(HTTPDNS_LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define httpdns_log_info(...)  httpdns_log_log(HTTPDNS_LOG_INFO,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define httpdns_log_warn(...)  httpdns_log_log(HTTPDNS_LOG_WARN,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define httpdns_log_error(...) httpdns_log_log(HTTPDNS_LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define httpdns_log_fatal(...) httpdns_log_log(HTTPDNS_LOG_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)

const char *httpdns_log_level_string(int level);

void httpdns_log_set_lock(httpdns_log_lock_func_t fn, void *udata);

void httpdns_log_set_level(int level);

void httpdns_log_set_quiet(bool enable);

int httpdns_log_add_callback(httpdns_log_log_func_t fn, void *udata, int level);

int httpdns_log_add_fp(FILE *fp, int level);

void httpdns_log_log(int level, const char *file, int line, const char *func, const char *fmt, ...);

/**
 * must free using httpdns_log_stop
 */
void httpdns_log_start();

void httpdns_log_stop();

#ifdef __cplusplus
}
#endif

#endif //HTTPDNS_C_SDK_HTTPDNS_LOG_H
