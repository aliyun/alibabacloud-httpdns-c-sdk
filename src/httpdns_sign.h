//
// Created by cagaoshuai on 2024/1/19.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_SIGN_H
#define HTTPDNS_C_SDK_HTTPDNS_SIGN_H

#include "openssl/md5.h"
#include <string.h>
#include  <stdio.h>
#include <stdint.h>

#define EXPIRATION_TIME 30 * 60

typedef struct {
    char *raw;
    char *sign;
    char *timestamp;
} httpdns_signature_t;

httpdns_signature_t *create_httpdns_signature(const char *host, const char *secret);

void destroy_httpdns_signature(httpdns_signature_t *signature);

#endif //HTTPDNS_C_SDK_HTTPDNS_SIGN_H
