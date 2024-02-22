//
// Created by caogaoshuai on 2024/1/9.
// 参考log.c https://github.com/rxi/log.c
//

#include "httpdns_sds.h"

#include "httpdns_log.h"

#define HTTPDNS_MAX_CALLBACKS 32

typedef struct {
  httpdns_log_log_func_t fn;
  void *udata;
  int level;
} httpdns_log_callback_t;

static struct {
  void *udata;
  httpdns_log_lock_func_t lock;
  int level;
  bool quiet;
  httpdns_log_callback_t callbacks[HTTPDNS_MAX_CALLBACKS];
} L;


static volatile FILE *log_file = NULL;

static const char *level_strings[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};
#endif


static void stdout_callback(httpdns_log_event_t *ev) {
  char buf[16];
  buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
#ifdef LOG_USE_COLOR
  fprintf(
    ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
    buf, level_colors[ev->level], level_strings[ev->level],
    ev->file, ev->line);
#else
  fprintf(
    ev->udata, "%s %-5s %s:%d: ",
    buf, level_strings[ev->level], ev->file, ev->line);
#endif
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}


static void file_callback(httpdns_log_event_t *ev) {
  char buf[64];
  buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
  fprintf(
    ev->udata, "%s %-5s %s:%d:%s: ",
    buf, level_strings[ev->level], ev->file, ev->line, ev->func);
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}


static void lock(void)   {
  if (L.lock) { L.lock(true, L.udata); }
}


static void unlock(void) {
  if (L.lock) { L.lock(false, L.udata); }
}


const char* httpdns_log_level_string(int level) {
  return level_strings[level];
}


void httpdns_log_set_lock(httpdns_log_lock_func_t fn, void *udata) {
  L.lock = fn;
  L.udata = udata;
}


void httpdns_log_set_level(int level) {
  L.level = level;
}


void httpdns_log_set_quiet(bool enable) {
  L.quiet = enable;
}


int httpdns_log_add_callback(httpdns_log_log_func_t fn, void *udata, int level) {
  for (int i = 0; i < HTTPDNS_MAX_CALLBACKS; i++) {
    if (!L.callbacks[i].fn) {
      L.callbacks[i] = (httpdns_log_callback_t) { fn, udata, level };
      return 0;
    }
  }
  return -1;
}


int httpdns_log_add_fp(FILE *fp, int level) {
  return httpdns_log_add_callback(file_callback, fp, level);
}


static void init_event(httpdns_log_event_t *ev, void *udata) {
  if (!ev->time) {
    time_t t = time(NULL);
    ev->time = localtime(&t);
  }
  ev->udata = udata;
}


void httpdns_log_log(int level, const char *file, int line, const char * func, const char *fmt, ...) {
  httpdns_log_event_t ev = {
    .fmt   = fmt,
    .file  = file,
    .line  = line,
    .level = level,
    .func = func
  };

  lock();

  if (!L.quiet && level >= L.level) {
    init_event(&ev, stderr);
    va_start(ev.ap, fmt);
    stdout_callback(&ev);
    va_end(ev.ap);
  }

  for (int i = 0; i < HTTPDNS_MAX_CALLBACKS && L.callbacks[i].fn; i++) {
    httpdns_log_callback_t *cb = &L.callbacks[i];
    if (level >= cb->level) {
      init_event(&ev, cb->udata);
      va_start(ev.ap, fmt);
      cb->fn(&ev);
      va_end(ev.ap);
    }
  }

  unlock();
}

void httpdns_log_start() {
    if (NULL != log_file) {
        return;
    }
    httpdns_log_set_level(HTTPDNS_LOG_LEVEL);
    httpdns_log_set_quiet(true);
    char log_file_path[1024] = HTTPDNS_MICRO_TO_STRING(HTTPDNS_LOG_FILE_PATH);
    if (strlen(log_file_path) <= 0) {
        httpdns_log_info("log file path is not set");
    }
    log_file = fopen(log_file_path, "ab+");
    if (NULL == log_file) {
        httpdns_log_error("open log file failed, file path %s", log_file_path);
    } else {
        httpdns_log_info("open log file success, file path %s", log_file_path);
        httpdns_log_add_fp(log_file, HTTPDNS_LOG_TRACE);
    }
}

void httpdns_log_stop() {
    if (NULL != log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}