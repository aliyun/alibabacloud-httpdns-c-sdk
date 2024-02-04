//
// Created by caogaoshuai on 2024/1/9.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_CONFIG_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_CONFIG_H

#include "list.h"
#include "net_stack_detector.h"
#include "httpdns_error_type.h"
#include <stdint.h>
#include <stdbool.h>

#define SDK_VERSION                "1.0.0"
#define USER_AGENT                 "emas-httpdns-c-sdk"
#define DEFAULT_IPV4_BOOT_SERVER   "203.107.1.1"
#define DEFAULT_IPV6_BOOT_SERVER   "2401:b180:2000:20::10"
#define DEFAULT_TIMEOUT_MS         3000L
#define DEFAULT_RETRY_TIMES        1
#define REGION_CHINA_MAINLAND      "cn"
#define REGION_HONG_KONG           "hk"
#define REGION_SINGAPORE           "sg"


typedef struct {
    char *account_id;
    char *secret_key;
    char *probe_domain;  //default www.taobao.com
    char *region;  //default cn, cn china ; sg singapore; hk HongKong
    int32_t timeout_ms; // max timeout in the whole process of http request, default 5000 ms, max value 5000
    int32_t retry_times; // retry times when one of servers failed
    char *sdk_version;
    char *user_agent;
    bool using_async;  // default true, false synchronously, true asynchronously
    bool using_cache;  // default true, false without cache, true with cache
    bool using_https;  // default false, false http, true https
    bool using_sign;   // default false, false not sign, true use sign
    bool fallbacking_localdns;  // default true, false not fallback, true fallback localdns
    struct list_head pre_resolve_hosts;
    struct list_head ipv4_boot_servers;
    struct list_head ipv6_boot_servers;
} httpdns_config_t;


/**
 * @description create a empty httpdns client config, should set config params manually
 * @return empty httpdns clent config
 * @note must free using httpdns_config_free
 */
httpdns_config_t *httpdns_config_new();


/**
 * @description set httpdns config account id
 * @note account_id is HTTPDNS account id, not aliyun User ID
 * @param config
 * @param account_id HTTPDNS account_id
 * @return: HTTPDNS_SUCCESS represents success, others represent specific failure
 */
int32_t httpdns_config_set_account_id(httpdns_config_t *config, const char *account_id);


/**
 * @description set httpdns config secret key
 * @note if secret key is setted, then set using_sign true
 * @param config
 * @param secret_key HTTPDNS secret key
 * @return: HTTPDNS_SUCCESS represents success, others represent specific failure
 */
int32_t httpdns_config_set_secret_key(httpdns_config_t *config, const char *secret_key);

int32_t httpdns_config_set_net_probe_domain(httpdns_config_t *config, const char *probe_domain);

/**
 * @description set the maximum timeout for requests to the httpdns server
 * @note when it exceeds 5000 ms, assign a value of 5000 ms
 * @param config
 * @param timeout_ms timeout interval
 * @return: HTTPDNS_SUCCESS represents success, others represent specific failure
 */
int32_t httpdns_config_set_timeout_ms(httpdns_config_t *config, int32_t timeout_ms);


/**
 * @description determine whether to parse asynchronously
 * @param config
 * @param using_async default 1, 0 synchronously, 1 asynchronously
 * @return: HTTPDNS_SUCCESS represents success, others represent specific failure
 */
int32_t httpdns_config_set_using_async(httpdns_config_t *config, bool using_async);

/**
 * @description determine whether to sign HTTP requests
 * @param config
 * @param using_cache default use cache, 0 not use cache, 1 use cache
 * @return: HTTPDNS_SUCCESS represents success, others represent specific failure
 */
int32_t httpdns_config_set_using_cache(httpdns_config_t *config, bool using_cache);

/**
 * @description determine whether to use the HTTPS protocol when requesting the httpdns server
 * @param config
 * @param using_https default http, 0 http, 1 https
 * @return: HTTPDNS_SUCCESS represents success, others represent specific failure
 */
int32_t httpdns_config_set_using_https(httpdns_config_t *config, bool using_https);


/**
 * @description determine whether to sign HTTP requests
 * @param config
 * @param using_https default not sign, 0 not sign, 1 sign
 * @return: HTTPDNS_SUCCESS represents success, others represent specific failure
 */
int32_t httpdns_config_set_using_sign(httpdns_config_t *config, bool using_sign);


/**
 * @description determine whether to fall back to localdns when accessing httpdns fails
 * @param config
 * @param using_https default 1, 0 not fallback, 1 fallback localdns
 * @return: HTTPDNS_SUCCESS represents success, others represent specific failure
 */
int32_t httpdns_config_set_fallbacking_localdns(httpdns_config_t *config, bool fallbacking_localdns);

/**
 * @description add pre-resolve host name
 * @param config
 * @param host host name
 * @return: HTTPDNS_SUCCESS represents success, others represent specific failure
 */
int32_t httpdns_config_add_pre_resolve_host(httpdns_config_t *config, const char *host);

/**
 * @description add ipv4 boot server
 * @param config
 * @param boot_server
 * @return: HTTPDNS_SUCCESS represents success, others represent specific failure
 */
int32_t httpdns_config_add_ipv4_boot_server(httpdns_config_t *config, const char *boot_server);

/**
 * @description add ipv6 boot server
 * @param config
 * @param boot_server
 * @return: HTTPDNS_SUCCESS represents success, others represent specific failure
 */
int32_t httpdns_config_add_ipv6_boot_server(httpdns_config_t *config, const char *boot_server);

int32_t httpdns_config_set_retry_times(httpdns_config_t *config, int32_t retry_times);

/**
 * check if given config is valid
 * @param config
 * @return 1 valid, 0 invalid
 */
int32_t httpdns_config_valid(httpdns_config_t *config);


/**
 * destroy config, this will free all memory allocated by this config
 * @param config
 */
void httpdns_config_free(httpdns_config_t *config);


#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_CONFIG_H
