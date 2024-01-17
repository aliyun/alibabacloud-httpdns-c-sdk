//
// Created by cagaoshuai on 2024/1/10.
//

#ifndef ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SCHEDULER_H
#define ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SCHEDULER_H

#include "httpdns_list.h"
#include "httpdns_http.h"
#include "cJSON.h"
#include "httpdns_config.h"
#include <stdint.h>


#define JSON_BODY_IPV4_RESOLVERS_ITEM  "service_ip"
#define JSON_BODY_IPV6_RESOLVERS_ITEM  "service_ipv6"
#define DEFAULT_RESOLVER_WEIGHT        0

typedef struct _httpdns_scheduler {
    struct list_head ipv4_resolve_servers;
    struct list_head ipv6_resolve_servers;
    net_stack_detector_t *net_stack_detector;
    httpdns_config_t *config;
} httpdns_scheduler_t;


typedef struct _httpdns_resolve_server {
    char *server;
    int32_t weight;
} httpdns_resolve_server_t;

httpdns_resolve_server_t *create_httpdns_resolve_server(char *server);

void destroy_httpdns_resolve_server(httpdns_resolve_server_t *resolve_server);

httpdns_scheduler_t *create_httpdns_scheduler(httpdns_config_t *config);

int32_t httpdns_scheduler_refresh_resolve_servers(httpdns_scheduler_t *scheduler);

void httpdns_scheduler_get_resolve_server(httpdns_scheduler_t *scheduler, char **resolve_server_ptr);

/*
 * config需要单独释放
 */
void destroy_httpdns_scheduler(httpdns_scheduler_t *scheduler);

#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SCHEDULER_H
