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
#define DEFAULT_RESOLVE_SERVER_RT        0
#define DELTA_WEIGHT_UPDATE_RATION     0.3

typedef struct {
    struct list_head ipv4_resolve_servers;
    struct list_head ipv6_resolve_servers;
    net_stack_detector_t *net_stack_detector;
    httpdns_config_t *config;
} httpdns_scheduler_t;


typedef struct {
    char *server;
    int32_t response_time_ms;
} httpdns_resolve_server_t;

httpdns_resolve_server_t *create_httpdns_resolve_server(char *server);

void httpdns_resolve_server_print(httpdns_resolve_server_t *resolve_server);

void destroy_httpdns_resolve_server(httpdns_resolve_server_t *resolve_server);

int32_t compare_httpdns_resolve_server(httpdns_resolve_server_t *server1, httpdns_resolve_server_t *server2);

httpdns_scheduler_t *create_httpdns_scheduler(httpdns_config_t *config);

void httpdns_scheduler_update_server_rt(httpdns_scheduler_t* scheduler, char *resolve_server_name, int32_t new_time_cost_ms);

int32_t httpdns_scheduler_refresh_resolve_servers(httpdns_scheduler_t *scheduler);

void httpdns_scheduler_get_resolve_server(httpdns_scheduler_t *scheduler, char **resolve_server_ptr);

void httpdns_scheduler_print_resolve_servers(httpdns_scheduler_t *scheduler);

/*
 * config需要单独释放
 */
void destroy_httpdns_scheduler(httpdns_scheduler_t *scheduler);



#endif //ALICLOUD_HTTPDNS_SDK_C_HTTPDNS_SCHEDULER_H
