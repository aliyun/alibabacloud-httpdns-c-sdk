//
// Created by cagaoshuai on 2024/1/31.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_STRING_H
#define HTTPDNS_C_SDK_HTTPDNS_STRING_H

#include <string.h>
#include "sds.h"

#define IS_NOT_BLANK_STRING(str) \
    (NULL != str && strlen(str) >0)

#define IS_BLANK_STRING(str) \
    ((NULL == str || strlen(str) <=0))

#define SDS_CAT(dst_str, slice) \
    dst_str = sdscat(dst_str, slice)


#define SDS_CAT_INT(dst_str, int_val) \
    {                                 \
        char tmp_buffer[26];                  \
        sprintf(tmp_buffer, "%d", int_val); \
        SDS_CAT(dst_str, tmp_buffer); \
    }

#define MICRO_STRINGIFY(x) #x
#define MICRO_TO_STRING(x) MICRO_STRINGIFY(x)

#endif //HTTPDNS_C_SDK_HTTPDNS_STRING_H
