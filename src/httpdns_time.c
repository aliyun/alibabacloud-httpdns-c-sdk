//
// Created by caogaoshuai on 2024/1/18.
//
#include "httpdns_time.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "log.h"


httpdns_sds_t httpdns_time_to_string(struct timeval ts) {
    char ts_str[32];
    time_t sec = ts.tv_sec;
    setenv("TZ", "GMT-8", 1);
    tzset();
    struct tm *tm_local = localtime(&sec);
    strftime(ts_str, 32, "%Y-%m-%d %H:%M:%S", tm_local);
    log_debug("timeval(tv_sec=%d,tv_usec=%d) to string is %s", ts.tv_sec, ts.tv_usec, ts_str);
    return httpdns_sds_new(ts_str);
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

int64_t httpdns_time_diff(struct timeval time1, struct timeval time2) {
    return (time1.tv_sec - time2.tv_sec) * 1000 + (time1.tv_usec - time2.tv_usec) / 1000;
}
