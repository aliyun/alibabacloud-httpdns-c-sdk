//
// Created by cagaoshuai on 2024/1/18.
//
#include "httpdns_time.h"
#include <stdio.h>
#include <stdbool.h>

void httpdns_time_to_string(struct timespec ts, char *buffer) {
    time_t sec = ts.tv_sec;
    struct tm *tm_local = localtime(&sec);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_local);
    printf("%s", buffer);
}

struct timespec httpdns_time_now() {
    struct timespec time_stamp;
    clock_gettime(CLOCK_REALTIME, &time_stamp);
    return time_stamp;
}

bool httpdns_time_is_expired(struct timespec ts, int32_t ttl) {
    struct timespec now = httpdns_time_now();
    ts.tv_sec = ts.tv_sec + ttl;
    if (ts.tv_sec > now.tv_sec) {
        return false;
    }
    if (ts.tv_sec < now.tv_sec) {
        return true;
    }
    return ts.tv_nsec >= now.tv_nsec;
}