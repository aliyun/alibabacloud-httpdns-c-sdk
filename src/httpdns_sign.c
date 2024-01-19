//
// Created by cagaoshuai on 2024/1/19.
//

#include "httpdns_sign.h"
#include "httpdns_time.h"
#include "sds.h"
#include "openssl/md5.h"
#include <string.h>
#include <stdlib.h>


static void uchar_to_hex_str(const unsigned char *in, size_t in_size, char *out) {
    const char hex_digits[] = "0123456789abcdef";
    for (int32_t i = 0; i < in_size; i++) {
        out[i * 2] = hex_digits[in[i] >> 4];
        out[i * 2 + 1] = hex_digits[in[i] & 0x0F];
    }
    out[in_size * 2] = '\0';
}

httpdns_signature_t *create_httpdns_signature(const char *host, const char *secret) {
    if (NULL == host || NULL == secret) {
        return NULL;
    }
    struct timespec ts = httpdns_time_now();
    char ts_str[32];
    sprintf(ts_str, "%ld", ts.tv_sec + EXPIRATION_TIME);
    sds raw_str = sdsnew(host);
    raw_str = sdscat(raw_str, "-");
    raw_str = sdscat(raw_str, secret);
    raw_str = sdscat(raw_str, "-");
    raw_str = sdscat(raw_str, ts_str);
    char raw_sign_str[16];
    MD5((unsigned char *) raw_str, strlen(raw_str), (unsigned char *) raw_sign_str);
    httpdns_signature_t *signature = (httpdns_signature_t *) malloc(sizeof(httpdns_signature_t));
    char hex_sign_str[33];
    uchar_to_hex_str((unsigned char *) raw_sign_str, 16, hex_sign_str);
    signature->sign = sdsnew(hex_sign_str);
    signature->timestamp = sdsnew(ts_str);
    signature->raw = raw_str;
    return signature;
}

void destroy_httpdns_signature(httpdns_signature_t *signature) {
    if (NULL == signature) {
        return;
    }
    if (NULL != signature->sign) {
        sdsfree(signature->timestamp);
    }
    if (NULL != signature->sign) {
        sdsfree(signature->sign);
    }
    if (NULL != signature->raw) {
        sdsfree(signature->raw);
    }
}