//
// Created by caogaoshuai on 2024/1/19.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_MEMORY_H
#define HTTPDNS_C_SDK_HTTPDNS_MEMORY_H

#ifdef __cplusplus
extern "C"
{
#endif

#define  HTTPDNS_NEW_OBJECT_IN_HEAP(var_name, type) \
    type* var_name = (type*)malloc(sizeof(type)); \
    memset(var_name, 0, sizeof (type))

#define HTTPDNS_SET_STRING_FIELD(object, field, value) \
    if (NULL != object && NULL != value) {         \
        if (NULL != object->field) {    \
            httpdns_sds_free(object->field);     \
        }                                \
        object->field = httpdns_sds_new(value);    \
    }



#ifdef __cplusplus
}
#endif
#endif //HTTPDNS_C_SDK_HTTPDNS_MEMORY_H
