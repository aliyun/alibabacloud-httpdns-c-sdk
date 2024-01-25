//
// Created by cagaoshuai on 2024/1/18.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_TIME_H
#define HTTPDNS_C_SDK_HTTPDNS_TIME_H

#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>

void httpdns_time_to_string(struct timeval ts, char *buffer, size_t size);

struct timeval httpdns_time_now();

bool httpdns_time_is_expired(struct timeval ts, int32_t ttl);

#endif //HTTPDNS_C_SDK_HTTPDNS_TIME_H
