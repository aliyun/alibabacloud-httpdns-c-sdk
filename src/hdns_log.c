#include "hdns_log.h"
#include "apr_portable.h"

hdns_log_print_pt hdns_log_print = hdns_log_print_default;
hdns_log_format_pt hdns_log_format = hdns_log_format_default;
hdns_log_level_e hdns_log_level = HDNS_LOG_WARN;
apr_file_t *hdns_stdout_file = NULL;


static const char *level_strings[] = {
        "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"
};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {
  "\x1b[35m", "\x1b[31m", "\x1b[33m", "\x1b[32m", "\x1b[36m", "\x1b[94m"
};
#endif


void hdns_log_set_print(hdns_log_print_pt p) {
    hdns_log_print = p;
}

void hdns_log_set_format(hdns_log_format_pt p) {
    hdns_log_format = p;
}

void hdns_log_set_level(hdns_log_level_e level) {
    hdns_log_level = level;
}

void hdns_log_set_output(apr_file_t *output) {
    hdns_stdout_file = output;
}

void hdns_log_print_default(const char *message, int len) {
    if (hdns_stdout_file == NULL) {
        fprintf(stdout, "%s", message);
    } else {
        apr_size_t bnytes = len;
        apr_file_write(hdns_stdout_file, message, &bnytes);
    }
}

void hdns_log_format_default(int level,
                             const char *file,
                             int line,
                             const char *function,
                             const char *fmt, ...) {
    int len;
    apr_time_t t;
    int s;
    apr_time_exp_t tm;
    va_list args;
    char buffer[4096];

    t = apr_time_now();
    if ((s = apr_time_exp_lt(&tm, t)) != APR_SUCCESS) {
        return;
    }

#ifdef LOG_USE_COLOR
    len = apr_snprintf(buffer, 4090, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] %" APR_INT64_T_FMT " %s%-5s\x1b[0m %s:%d ",
                       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                       tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_usec / 1000,
                       (int64_t) apr_os_thread_current(), level_colors[level], level_strings[level], file, line);
#else
    len = apr_snprintf(buffer, 4090, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] %" APR_INT64_T_FMT " %-5s %s:%d ",
                       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                       tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_usec / 1000,
                       (int64_t) apr_os_thread_current(), level_strings[level], file, line);
#endif
    va_start(args, fmt);
    len += vsnprintf(buffer + len, 4090 - len, fmt, args);
    va_end(args);

    while (buffer[len - 1] == '\n') len--;
    buffer[len++] = '\n';
    buffer[len] = '\0';

    hdns_log_print(buffer, len);
}

