//
// Created by cagaoshuai on 2024/1/9.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H

#include "httpdns_scheduler.h"
#include "httpdns_config.h"
#include "httpdns_cache.h"
#include "httpdns_result.h"

typedef struct _httpdns_resolver {
    httpdns_scheduler_t scheduler;
    httpdns_config_t config;
    httpdns_cache_t cache;
} httpdns_resolver_t;

typedef enum _dns_type {
    A,
    AAAA,
    BOTH,
    AUTO
} dns_type_t;

/**
 * @description create httpdns resolver
 * @param config httpdns config
 * @return:  resolver
 */
httpdns_resolver_t *create_httpdns_resolver(httpdns_config_t *config);

/**
 * @description resolve domain
 * @param resolver resolver
 * @param host domain to be resolve
 * @param dns_type target DNS record type
 * @return:  httpdns_generic_result_t
 */
httpdns_generic_result_t get_httpdns_result_for_host(httpdns_resolver_t *resolver, const char *host, dns_type_t dns_type);

/**
 * @description resolve domain synchronously and do not refer to cached results
 * @param resolver resolver
 * @param host domain to be resolve
 * @param dns_type target DNS record type
 * @return:  httpdns_generic_result_t
 */
httpdns_generic_result_t get_httpdns_result_for_host_with_sdns(httpdns_resolver_t *resolver, const char *host, dns_type_t dns_type, dict params,
                                      const char *cache_key);

/**
 * destroy resolver, this will free all memory allocated by this config
 * @param config
 */
void destroy_create_httpdns_resolver(httpdns_resolver_t *resolver);

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_RESOLVER_H
