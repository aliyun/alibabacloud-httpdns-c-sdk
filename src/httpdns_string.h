//
// Created by caogaoshuai on 2024/1/31.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_STRING_H
#define HTTPDNS_C_SDK_HTTPDNS_STRING_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <string.h>
#include "httpdns_sds.h"

#define IS_NOT_BLANK_STRING(str) \
    (NULL != str && strlen(str) >0)

#define IS_BLANK_STRING(str) \
    ((NULL == str || strlen(str) <=0))

#define SDS_CAT(dst_str, slice) \
    dst_str = httpdns_sds_cat(dst_str, slice)


#define SDS_CAT_INT(dst_str, int_val) \
    {                                 \
        char tmp_buffer[26];                  \
        sprintf(tmp_buffer, "%d", int_val); \
        SDS_CAT(dst_str, tmp_buffer); \
    }
#define SDS_CAT_CHAR(dst_str, ch_val) \
    {                                 \
        char tmp_buffer[2];           \
        tmp_buffer[0]=ch_val;         \
        tmp_buffer[1]='\0';         \
        SDS_CAT(dst_str, tmp_buffer); \
    }

#define MICRO_STRINGIFY(x) #x
#define MICRO_TO_STRING(x) MICRO_STRINGIFY(x)

#ifdef __cplusplus
}
#endif

#endif //HTTPDNS_C_SDK_HTTPDNS_STRING_H
