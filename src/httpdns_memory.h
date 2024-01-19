//
// Created by cagaoshuai on 2024/1/19.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_MEMORY_H
#define HTTPDNS_C_SDK_HTTPDNS_MEMORY_H

#define  HTTPDNS_NEW_OBJECT_IN_HEAP(var_name, type) \
    type* var_name = (type*)malloc(sizeof(type)); \
    memset(var_name, 0, sizeof (type))


#endif //HTTPDNS_C_SDK_HTTPDNS_MEMORY_H
