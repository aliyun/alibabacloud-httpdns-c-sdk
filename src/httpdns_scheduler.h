//
// Created by cagaoshuai on 2024/1/10.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SCHEDULER_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SCHEDULER_H

#include "httpdns_list.h"
#include "httpdns_http.h"
#include "httpdns_config.h"
#include <stdint.h>


#define DELTA_WEIGHT_UPDATE_RATION     0.3
#define SCHEDULE_NONCE_SIZE            12

typedef struct {
    struct list_head ipv4_resolve_servers;
    struct list_head ipv6_resolve_servers;
    httpdns_net_stack_detector_t *net_stack_detector;
    httpdns_config_t *config;
} httpdns_scheduler_t;


httpdns_scheduler_t *httpdns_scheduler_create(httpdns_config_t *config);

int32_t httpdns_scheduler_refresh(httpdns_scheduler_t *scheduler);

void httpdns_scheduler_update(httpdns_scheduler_t *scheduler, char *server, int32_t rt);

char *httpdns_scheduler_get(httpdns_scheduler_t *scheduler);

void httpdns_scheduler_print(httpdns_scheduler_t *scheduler);

void httpdns_scheduler_destroy(httpdns_scheduler_t *scheduler);

void httpdns_scheduler_set_net_stack_detector(httpdns_scheduler_t *scheduler, httpdns_net_stack_detector_t *net_stack_detector);

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SCHEDULER_H
