//
// HTTPDNS组件集成，实现HTTPDNS解析功能，组件包括
// 网络环境检测、解析器、缓存、调度器
//
// Created by caogaoshuai on 2024/1/22.
//

#ifndef HDNS_C_SDK_HDNS_CLIENT_H
#define HDNS_C_SDK_HDNS_CLIENT_H


#include "hdns_cache.h"
#include "hdns_resolver.h"
#include "hdns_scheduler.h"
#include "hdns_define.h"
#include "apr_thread_pool.h"

HDNS_CPP_START

typedef struct {
    hdns_pool_t *pool;
    hdns_scheduler_t *scheduler;
    hdns_net_detector_t *net_detector;
    hdns_config_t *config;
    hdns_cache_t *cache;
    hdns_state_e state;
} hdns_client_t;


hdns_status_t hdns_do_single_resolve(hdns_client_t *client,
                                     const char *host,
                                     hdns_query_type_t query_type,
                                     bool using_cache,
                                     const char *client_ip,
                                     hdns_list_head_t *results);

hdns_status_t hdns_do_batch_resolve(hdns_client_t *client,
                                    const hdns_list_head_t *hosts,
                                    hdns_query_type_t query_type,
                                    bool using_cache,
                                    const char *client_ip,
                                    hdns_list_head_t *results);

hdns_status_t hdns_do_single_resolve_with_req(hdns_client_t *client,
                                              hdns_resv_req_t *req,
                                              hdns_list_head_t *results);

void hdns_update_cache_on_net_change(hdns_net_chg_cb_task_t * task);

HDNS_CPP_END

#endif
