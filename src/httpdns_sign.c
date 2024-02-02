//
// Created by caogaoshuai on 2024/1/19.
//

#include "httpdns_sign.h"
#include "httpdns_time.h"
#include "sds.h"
#include "openssl/md5.h"
#include <string.h>
#include <stdlib.h>
#include "httpdns_memory.h"
#include "log.h"


static void uchar_to_hex_str(const unsigned char *in, char *out) {
    const char hex_digits[] = "0123456789abcdef";
    for (int32_t i = 0; i < 16; i++) {
        out[i * 2] = hex_digits[in[i] >> 4];
        out[i * 2 + 1] = hex_digits[in[i] & 0x0F];
    }
    out[16 * 2] = '\0';
}

httpdns_signature_t *httpdns_signature_new(const char *host, const char *secret, int32_t max_offset, struct timeval tv) {
    if (NULL == host || NULL == secret) {
        return NULL;
    }
    char ts_str[32];
    sprintf(ts_str, "%ld", tv.tv_sec + max_offset);
    sds raw_str = sdsnew(host);
    raw_str = sdscat(raw_str, "-");
    raw_str = sdscat(raw_str, secret);
    raw_str = sdscat(raw_str, "-");
    raw_str = sdscat(raw_str, ts_str);
    char raw_sign_str[16];
    MD5((unsigned char *) raw_str, strlen(raw_str), (unsigned char *) raw_sign_str);
    HTTPDNS_NEW_OBJECT_IN_HEAP(signature, httpdns_signature_t);
    char hex_sign_str[33];
    uchar_to_hex_str((unsigned char *) raw_sign_str, hex_sign_str);
    signature->sign = sdsnew(hex_sign_str);
    signature->timestamp = sdsnew(ts_str);
    signature->raw = raw_str;
    log_debug("httpdns_signature_t(raw=%s,timestamp=%s,sign=%s)", signature->raw, signature->timestamp, signature->sign);
    return signature;
}

void httpdns_signature_free(httpdns_signature_t *signature) {
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
    free(signature);
}