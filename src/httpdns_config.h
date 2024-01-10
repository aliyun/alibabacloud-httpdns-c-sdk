//
// Created by cagaoshuai on 2024/1/9.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_CONFIG_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_CONFIG_H

#include "../libs/list.h"
#include "httpdns_error_type.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct _httpdns_config {
    char *account_id;
    char *secret_key;
    char *region;  //default cn, cn china ; sg singapore; hk HongKong
    int64_t timeout_ms; // max timeout in the whole process of http request, default 5000 ms, max value 5000
    bool using_async;  // default 1, 0 synchronously, 1 asynchronously
    bool using_cache;  // default 1, 0 without cache, 1 with cache
    bool using_https;  // default 0, 0 http, 1 https
    bool using_sign;   // default 0, 0 not sign, 1 use sign
    bool fallbacking_localdns;  // default 1, 0 not fallback, 1 fallback localdns
} httpdns_config_t;


/**
 * @description create a empty httpdns client config, should set config params manually
 * @return empty httpdns clent config
 */
httpdns_config_t *create_httpdns_config();


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


/**
 * @description set the maximum timeout for requests to the httpdns server
 * @note when it exceeds 5000 ms, assign a value of 5000 ms
 * @param config
 * @param timeout_ms timeout interval
 * @return: HTTPDNS_SUCCESS represents success, others represent specific failure
 */
int32_t httpdns_config_set_timeout_ms(httpdns_config_t *config, int64_t timeout_ms);


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
int32_t httpdns_config_set_using_sign(httpdns_config_t *config, bool fallbacking_localdns);

/**
 * check if given config is valid
 * @param config
 * @return 1 valid, 0 invalid
 */
int32_t httpdns_config_is_valid(httpdns_config_t *config);


/**
 * destroy config, this will free all memory allocated by this config
 * @param config
 */
void destroy_httpdns_config(httpdns_config_t *config);


#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_CONFIG_H
