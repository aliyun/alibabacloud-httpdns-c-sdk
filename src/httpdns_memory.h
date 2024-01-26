//
// Created by cagaoshuai on 2024/1/19.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_MEMORY_H
#define HTTPDNS_C_SDK_HTTPDNS_MEMORY_H

#define  HTTPDNS_NEW_OBJECT_IN_HEAP(var_name, type) \
    type* var_name = (type*)malloc(sizeof(type)); \
    memset(var_name, 0, sizeof (type))

#define HTTPDNS_SET_STRING_FIELD(object, field, value) \
    if (NULL != object && NULL != value) {         \
        if (NULL != object->field) {    \
            sdsfree(object->field);     \
        }                                \
        object->field = sdsnew(value);    \
    }

#define MUST_FREE __attribute__((warn_unused_result))
#endif //HTTPDNS_C_SDK_HTTPDNS_MEMORY_H
