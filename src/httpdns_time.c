//
// Created by cagaoshuai on 2024/1/18.
//
#include "httpdns_time.h"
#include <stdio.h>

void httpdns_time_to_string(struct timespec ts, char* buffer) {
    time_t sec = ts.tv_sec;
    struct tm *tm_local = localtime(&sec);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_local);
    printf("%s", buffer);
}