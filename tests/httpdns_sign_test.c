//
// Created by cagaoshuai on 2024/1/19.
//
#include "httpdns_sign.h"

int main(void) {
    httpdns_signature_t *signature = httpdns_signature_create("www.aliyun.com", "123456", MAX_RESOLVE_SIGNATURE_OFFSET_TIME);
    printf("raw:%s\nsign:%s\n", signature->raw, signature->sign);
    destroy_httpdns_signature(signature);
    return 0;
}
