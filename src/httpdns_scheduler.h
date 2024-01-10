//
// Created by cagaoshuai on 2024/1/10.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SCHEDULE_CLIENT_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SCHEDULE_CLIENT_H

#include "../libs/list.h"
#include <stdint.h>

typedef struct _httpdns_scheduler {
    struct list_head resolve_server_list;
    char * storeFile;

} httpdns_scheduler_t;


typedef struct _httpdns_resolve_server {
    char *server;
    int64_t connnect_time_cost;
} httpdns_resolve_server_t;


#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SCHEDULE_CLIENT_H
