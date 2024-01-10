//
// Created by cagaoshuai on 2024/1/9.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H

#include "../libs/list.h"

struct _httpdns_list_node {
    struct list_head list;
    void *data;
} httpdns_list_node_t;


#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_LIST_H
