//
// Created by caogaoshuai on 2024/1/19.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_SIGN_H
#define HTTPDNS_C_SDK_HTTPDNS_SIGN_H

#ifdef __cplusplus
extern "C"
{
#endif


#include "openssl/md5.h"
#include <string.h>
#include  <stdio.h>
#include <stdint.h>
#include "httpdns_time.h"

#define MAX_RESOLVE_SIGNATURE_OFFSET_TIME (30 * 60)
#define MAX_SCHEDULE_SIGNATURE_OFFSET_TIME 0

typedef struct {
    char *raw;
    char *sign;
    char *timestamp;
} httpdns_signature_t;

/**
 * must free using httpdns_signature_free
 */
httpdns_signature_t *httpdns_signature_new(const char *host, const char *secret, int32_t max_offset, struct timeval tv);

void httpdns_signature_free(httpdns_signature_t *signature);

#ifdef __cplusplus
}
#endif

#endif //HTTPDNS_C_SDK_HTTPDNS_SIGN_H
