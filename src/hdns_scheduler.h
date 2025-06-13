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


#define HDNS_SCHEDULE_NONCE_SIZE             12

typedef struct {
    hdns_pool_t *pool;
    hdns_list_head_t *ipv4_resolvers;
    int32_t cur_ipv4_resolver_index;
    hdns_list_head_t *ipv6_resolvers;
    int32_t cur_ipv6_resolver_index;
    apr_time_t    next_timer_refresh_time;
    bool is_refreshed;
    hdns_net_detector_t *detector;
    apr_thread_pool_t *thread_pool;
    hdns_config_t *config;
    apr_thread_mutex_t *lock;
    hdns_state_e state;
} hdns_scheduler_t;


hdns_scheduler_t *hdns_scheduler_create(hdns_config_t *config,
                                        hdns_net_detector_t *detector,
                                        apr_thread_pool_t *thread_pool);

hdns_status_t hdns_scheduler_refresh_async(hdns_scheduler_t *scheduler);

hdns_status_t hdns_scheduler_start_refresh_timer(hdns_scheduler_t *scheduler);

void hdns_scheduler_failover(hdns_scheduler_t *scheduler, const char *server);

int hdns_scheduler_get(hdns_scheduler_t *scheduler, char *resolver);

int hdns_scheduler_cleanup(hdns_scheduler_t *scheduler);

HDNS_CPP_END

#endif
