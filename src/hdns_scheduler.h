//
// Created by caogaoshuai on 2024/1/10.
//

#ifndef HDNS_C_SDK_HDNS_SCHEDULER_H
#define HDNS_C_SDK_HDNS_SCHEDULER_H

#include "apr_thread_pool.h"

#include "hdns_config.h"
#include "hdns_list.h"
#include "hdns_net.h"
#include "hdns_define.h"

HDNS_CPP_START


#define HDNS_DELTA_WEIGHT_UPDATE_RATION     0.3
#define HDNS_SCHEDULE_NONCE_SIZE             12

typedef struct {
    hdns_pool_t *pool;
    hdns_list_head_t *ipv4_resolvers;
    hdns_list_head_t *ipv6_resolvers;
    hdns_net_detector_t *detector;
    apr_thread_pool_t *thread_pool;
    hdns_config_t *config;
    apr_thread_mutex_t *lock;
} hdns_scheduler_t;


hdns_scheduler_t *hdns_scheduler_create(hdns_config_t *config,
                                        hdns_net_detector_t *detector,
                                        apr_thread_pool_t *thread_pool);

hdns_status_t hdns_scheduler_refresh_async(hdns_scheduler_t *scheduler);

void hdns_scheduler_update(hdns_scheduler_t *scheduler, const char *server, int32_t rt);

int hdns_scheduler_get(hdns_scheduler_t *scheduler, char *resolver);

int hdns_scheduler_cleanup(hdns_scheduler_t *scheduler);

HDNS_CPP_END

#endif
