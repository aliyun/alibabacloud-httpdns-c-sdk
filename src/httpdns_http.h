//
// Created by cagaoshuai on 2024/1/10.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_HTTP_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_HTTP_H

#include<stdint.h>

typedef struct _httpdns_http_context {
    char *  url;
    int64_t timeout_ms;
    char * response_body;
} httpdns_http_context_t;




#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_HTTP_H
