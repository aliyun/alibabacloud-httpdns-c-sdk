//
// Created by cagaoshuai on 2024/1/10.
//


#include "httpdns_config.h"
#include "httpdns_list.h"
#include <string.h>
#include <stdlib.h>


void _set_default_httpdns_config(httpdns_config_t *config_ptr) {
    config_ptr->using_async = true;
    config_ptr->using_cache = true;
    config_ptr->using_https = false;
    config_ptr->using_sign = false;
    config_ptr->fallbacking_localdns = true;
    config_ptr->timeout_ms = DEFAULT_TIMEOUT_MS;
    config_ptr->region = REGION_CHINA_MAINLAND;
    httpdns_list_init(&(config_ptr->pre_resolve_hosts));
    httpdns_list_init(&(config_ptr->boot_servers));
}

httpdns_config_t *create_httpdns_config() {
    httpdns_config_t *config_ptr = (httpdns_config_t *) malloc(sizeof(httpdns_config_t));
    memset(config_ptr, 0, sizeof(httpdns_config_t));
    _set_default_httpdns_config(config_ptr);
    return config_ptr;
}