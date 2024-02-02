//
// Created by caogaoshuai on 2024/1/29.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_RESOLVE_RESULT_H
#define HTTPDNS_C_SDK_HTTPDNS_RESOLVE_RESULT_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include "httpdns_list.h"
#include "sds.h"


typedef struct {
    char *host;
    char *client_ip;
    char *extra;
    struct list_head ips;
    struct list_head ipsv6;
    int origin_ttl;
    int ttl;
    struct timeval query_ts;
    void *cache_key;
    bool hit_cache;
} httpdns_resolve_result_t;


void httpdns_resolve_result_free(httpdns_resolve_result_t *result);

httpdns_resolve_result_t *httpdns_resolve_result_clone(const httpdns_resolve_result_t *origin_result);

sds httpdns_resolve_result_to_string(const httpdns_resolve_result_t *result);

void httpdns_resolve_result_set_cache_key(httpdns_resolve_result_t *result, const char *cache_key);

void httpdns_resolve_result_set_hit_cache(httpdns_resolve_result_t *result, bool hit_cache);

int32_t httpdns_resolve_result_cmp(const httpdns_resolve_result_t *result1, const httpdns_resolve_result_t *result2);

void httpdns_resolve_results_merge(struct list_head *raw_results, struct list_head *merged_results);


#endif //HTTPDNS_C_SDK_HTTPDNS_RESOLVE_RESULT_H
