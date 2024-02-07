//
// Created by caogaoshuai on 2024/1/18.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_TIME_H
#define HTTPDNS_C_SDK_HTTPDNS_TIME_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include "sds.h"

sds httpdns_time_to_string(struct timeval ts);

struct timeval httpdns_time_now();

bool httpdns_time_is_expired(struct timeval ts, int32_t ttl);

int64_t httpdns_time_diff(struct timeval time1, struct timeval time2);

#ifdef __cplusplus
}
#endif

#endif //HTTPDNS_C_SDK_HTTPDNS_TIME_H
