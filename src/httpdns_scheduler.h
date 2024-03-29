//
// Created by caogaoshuai on 2024/1/10.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SCHEDULER_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SCHEDULER_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>

#include "httpdns_client_config.h"
#include "httpdns_list.h"
#include "httpdns_net_stack_detector.h"

#define HTTPDNS_DELTA_WEIGHT_UPDATE_RATION     0.3
#define HTTPDNS_SCHEDULE_NONCE_SIZE            12

typedef struct {
    httpdns_list_head_t ipv4_resolve_servers;
    httpdns_list_head_t ipv6_resolve_servers;
    httpdns_net_stack_detector_t *net_stack_detector;
    httpdns_config_t *config;
    pthread_mutex_t lock;
    pthread_mutexattr_t lock_attr;
} httpdns_scheduler_t;

/**
 * must free using httpdns_scheduler_free
 */
httpdns_scheduler_t *httpdns_scheduler_new(httpdns_config_t *config);

int32_t httpdns_scheduler_refresh(httpdns_scheduler_t *scheduler);

void httpdns_scheduler_update(httpdns_scheduler_t *scheduler, const char *server, int32_t rt);

char *httpdns_scheduler_get(httpdns_scheduler_t *scheduler);

/**
 * must free using httpdns_sds_free
 */
httpdns_sds_t httpdns_scheduler_to_string(httpdns_scheduler_t *scheduler);

void httpdns_scheduler_free(httpdns_scheduler_t *scheduler);

void httpdns_scheduler_set_net_stack_detector(httpdns_scheduler_t *scheduler,
                                              httpdns_net_stack_detector_t *net_stack_detector);

void httpdns_scheduler_add_ipv4_resolve_server(httpdns_scheduler_t *scheduler,
                                              const char* resolve_server);

void httpdns_scheduler_add_ipv6_resolve_server(httpdns_scheduler_t *scheduler,
                                               const char* resolve_server);

#ifdef __cplusplus
}
#endif

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SCHEDULER_H
