//
// Created by caogaoshuai on 2024/1/9.
//

#ifndef HDNS_C_SDK_HDNS_CONFIG_H
#define HDNS_C_SDK_HDNS_CONFIG_H

#include "hdns_list.h"
#include "hdns_status.h"

HDNS_CPP_START

#define HDNS_REGION_NUM  5

typedef enum {
    HDNS_REGION_CN = 0,
    HDNS_REGION_HK = 1,
    HDNS_REGION_SG = 2,
    HDNS_REGION_US = 3,
    HDNS_REGION_DE = 4
} hdns_region_t;



typedef struct {
    hdns_pool_t *pool;
    char *account_id;
    char *secret_key;
    char *region;
    int32_t timeout;
    int32_t retry_times;
    bool using_cache;
    bool using_https;
    bool using_sign;
    bool enable_expired_ip;
    bool enable_failover_localdns;
    char* session_id;
    hdns_list_head_t *pre_resolve_hosts;
    hdns_list_head_t *ipv4_boot_servers;
    hdns_list_head_t *ipv6_boot_servers;
    hdns_hash_t *ip_probe_items;
    hdns_hash_t *custom_ttl_items;
    apr_thread_mutex_t *lock;
} hdns_config_t;


hdns_config_t *hdns_config_create();

hdns_status_t hdns_config_valid(hdns_config_t *config);

void hdns_config_cleanup(hdns_config_t *config);

HDNS_CPP_END

#endif
