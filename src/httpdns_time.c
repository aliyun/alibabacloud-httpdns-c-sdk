//
// Created by cagaoshuai on 2024/1/18.
//
#include "httpdns_time.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "log.h"


void httpdns_time_to_string(struct timeval ts, char *buffer, size_t size) {
    time_t sec = ts.tv_sec;
    setenv("TZ", "GMT-8", 1);
    tzset();
    struct tm *tm_local = localtime(&sec);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_local);
    log_debug("timeval(tv_sec=%d,tv_usec=%d) to string is %s", ts.tv_sec, ts.tv_usec, buffer);
}

struct timeval httpdns_time_now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    log_debug("now timeval(tv_sec=%d,tv_usec=%d)", tv.tv_sec, tv.tv_usec);
    return tv;
}

bool httpdns_time_is_expired(struct timeval ts, int32_t ttl) {
    struct timeval now = httpdns_time_now();
    ts.tv_sec = ts.tv_sec + ttl;
    if (ts.tv_sec > now.tv_sec) {
        return false;
    }
    if (ts.tv_sec < now.tv_sec) {
        return true;
    }
    return ts.tv_usec >= now.tv_usec;
}
