//
// Created by cagaoshuai on 2024/1/19.
//
#include "httpdns_sign.h"

int main(void) {
    httpdns_signature_t *signature = create_httpdns_signature("www.aliyun.com", "123456");
    printf("raw:%s\nsign:%s\n", signature->raw, signature->sign);
    destroy_httpdns_signature(signature);
    return 0;
}
