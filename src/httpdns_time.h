//
// Created by cagaoshuai on 2024/1/18.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_TIME_H
#define HTTPDNS_C_SDK_HTTPDNS_TIME_H

#include <time.h>
#include <stdbool.h>
#include <stdint.h>

void httpdns_time_to_string(struct timespec ts, char *buffer, size_t size);

struct timespec httpdns_time_now();

bool httpdns_time_is_expired(struct timespec ts, int32_t ttl);

#endif //HTTPDNS_C_SDK_HTTPDNS_TIME_H
