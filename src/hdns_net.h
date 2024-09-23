//
// Created by caogaoshuai on 2024/1/11.
//

#ifndef HDNS_C_SDK_HDNS_NET_H
#define HDNS_C_SDK_HDNS_NET_H



/**
 * @description Detect the type of network stack

 * @refer https://android.googlesource.com/platform/bionic/+/085543106/libc/results/net/getaddrinfo.c
 *
}
 */

#include <apr_thread_pool.h>
#include <apr_atomic.h>
#include <apr_thread_cond.h>

#include "hdns_list.h"
#include "hdns_define.h"

HDNS_CPP_START

#define HDNS_IPV4_PROBE_ADDR  "8.8.8.8"
#define HDNS_IPV6_PROBE_ADDR  "2000::"
#define HDNS_PROBE_DOMAIN     "www.taobao.com"
#define HDNS_PROBE_PORT        0xFFFF

typedef enum {
    HDNS_NET_UNKNOWN = 0x00,
    HDNS_IPV4_ONLY = 0x01,
    HDNS_IPV6_ONLY = 0x02,
    HDNS_DUAL_STACK = 0x03
} hdns_net_type_t;

typedef struct hdns_net_chg_cb_task_s hdns_net_chg_cb_task_t;

typedef void(*hdns_net_chg_cb_fn_t)(hdns_net_chg_cb_task_t *callback);

typedef void(*hdns_net_speed_cb_fn_t)(hdns_list_head_t *sorted_ips, void *user_params);

typedef enum {
    HDNS_NET_CB_UPDATE_NET_TYPE,
    HDNS_NET_CB_UPDATE_CACHE
} hdns_net_chg_cb_type_t;

struct hdns_net_chg_cb_task_s {
    hdns_net_chg_cb_fn_t fn;
    void *param;
    void *ownner;
    hdns_net_chg_cb_type_t type;
    volatile apr_uint32_t parallelism;
    volatile bool stop_signal;
};

typedef struct {
    hdns_pool_t *pool;
    hdns_net_speed_cb_fn_t fn;
    hdns_list_head_t *ips;
    int port;
    void *param;
    void *owner;
    volatile hdns_state_e *ownner_state;
} hdns_net_speed_detect_task_t;

typedef struct {
    // 只有读写，可以不加锁
    volatile hdns_net_type_t type;
} hdns_net_type_detector_t;

typedef struct {
    volatile bool is_first;
    volatile bool stop_signal;
    apr_thread_mutex_t *lock;
    hdns_list_head_t *local_ips;
    hdns_list_head_t *cb_tasks;
} hdns_net_change_detector_t;

typedef struct {
    volatile bool stop_signal;
    apr_thread_mutex_t *lock;
    apr_thread_cond_t *not_empty_cond;
    hdns_list_head_t *tasks;
} hdns_net_speed_detector_t;

typedef struct {
    hdns_pool_t *pool;
    hdns_net_type_detector_t *type_detector;
    hdns_net_change_detector_t *change_detector;
    hdns_net_speed_detector_t *speed_detector;
    apr_thread_pool_t *thread_pool;
} hdns_net_detector_t;


hdns_net_detector_t *hdns_net_detector_create(apr_thread_pool_t *thread_pool);

void hdns_net_detector_stop(hdns_net_detector_t *detector);

void hdns_net_detector_cleanup(hdns_net_detector_t *detector);

void hdns_net_add_chg_cb_task(hdns_net_detector_t *detector,
                              hdns_net_chg_cb_type_t type,
                              hdns_net_chg_cb_fn_t fn,
                              void *param,
                              void *owner);


void hdns_net_cancel_chg_cb_task(hdns_net_detector_t *detector, void *owner);

void hdns_net_add_speed_detect_task(hdns_net_detector_t *detector,
                                    hdns_net_speed_cb_fn_t fn,
                                    void *param,
                                    hdns_list_head_t *ips,
                                    int port,
                                    void *owner,
                                    hdns_state_e *owner_state);

hdns_net_type_t hdns_net_get_type(hdns_net_detector_t *detector);

bool hdns_net_is_changed(hdns_net_detector_t *detector);

HDNS_CPP_END

#endif
