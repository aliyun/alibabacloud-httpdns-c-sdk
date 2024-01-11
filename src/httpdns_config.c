//
// Created by cagaoshuai on 2024/1/10.
//


#include "httpdns_config.h"
#include "httpdns_list.h"
#include "../libs/sds.h"
#include <string.h>
#include <stdlib.h>


static void _set_default_httpdns_config(httpdns_config_t *config_ptr) {
    config_ptr->using_async = true;
    config_ptr->using_cache = true;
    config_ptr->using_https = false;
    config_ptr->using_sign = false;
    config_ptr->fallbacking_localdns = true;
    config_ptr->timeout_ms = DEFAULT_TIMEOUT_MS;
    config_ptr->region = sdsnew(REGION_CHINA_MAINLAND);
    httpdns_list_init(&config_ptr->pre_resolve_hosts);
    httpdns_list_init(&config_ptr->ipv4_boot_servers);
    httpdns_list_add(&config_ptr->ipv4_boot_servers, sdsnew(DEFAULT_IPV4_BOOT_SERVER));
    httpdns_list_init(&config_ptr->ipv6_boot_servers);
    httpdns_list_add(&config_ptr->ipv6_boot_servers, sdsnew(DEFAULT_IPV6_BOOT_SERVER));
}

httpdns_config_t *create_httpdns_config() {
    httpdns_config_t *config_ptr = (httpdns_config_t *) malloc(sizeof(httpdns_config_t));
    memset(config_ptr, 0, sizeof(httpdns_config_t));
    _set_default_httpdns_config(config_ptr);
    return config_ptr;
}


int32_t httpdns_config_set_account_id(httpdns_config_t *config, const char *account_id) {
    if (NULL == config || NULL == account_id) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->account_id = sdsnew(account_id);
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_set_secret_key(httpdns_config_t *config, const char *secret_key) {
    if (NULL == config || NULL == secret_key) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->secret_key = sdsnew(secret_key);
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_set_timeout_ms(httpdns_config_t *config, int64_t timeout_ms) {
    if (config == NULL || timeout_ms <= 0) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    config->timeout_ms = timeout_ms;
    return HTTPDNS_SUCCESS;
}


int32_t httpdns_config_set_using_async(httpdns_config_t *config, bool using_async) {
    if (NULL == config) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->using_async = using_async;
    return HTTPDNS_SUCCESS;
}


int32_t httpdns_config_set_using_cache(httpdns_config_t *config, bool using_cache) {
    if (NULL == config) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->using_cache = using_cache;
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_set_using_https(httpdns_config_t *config, bool using_https) {
    if (NULL == config) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->using_https = using_https;
    return HTTPDNS_SUCCESS;
}


int32_t httpdns_config_set_using_sign(httpdns_config_t *config, bool using_sign) {
    if (NULL == config) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->using_sign = using_sign;
    return HTTPDNS_SUCCESS;
}


int32_t httpdns_config_set_fallbacking_localdns(httpdns_config_t *config, bool fallbacking_localdns) {
    if (NULL == config) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->fallbacking_localdns = fallbacking_localdns;
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_add_pre_resolve_host(httpdns_config_t *config, const char *host) {
    if (NULL == config || NULL == host) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    httpdns_list_add(&config->pre_resolve_hosts, sdsnew(host));
    return HTTPDNS_SUCCESS;
}


int32_t httpdns_config_add_ipv4_boot_server(httpdns_config_t *config, const char *boot_server) {
    if (NULL == config || NULL == boot_server) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    httpdns_list_add(&config->ipv4_boot_servers, sdsnew(boot_server));
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_add_ipv6_boot_server(httpdns_config_t *config, const char *boot_server) {
    if (NULL == config || NULL == boot_server) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    httpdns_list_add(&config->ipv6_boot_servers, sdsnew(boot_server));
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_is_valid(httpdns_config_t *config) {
    if (NULL == config) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (sdslen(config->account_id) <= 0) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (config->using_sign && sdslen(config->secret_key) <= 0) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (httpdns_list_size(&config->ipv4_boot_servers) <= 0 && httpdns_list_size(&config->ipv6_boot_servers) <= 0) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (sdslen(config->region) <= 0) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (config->timeout_ms <= 0) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    return HTTPDNS_SUCCESS;
}

void destroy_httpdns_config(httpdns_config_t *config) {
    if (NULL == config) {
        return;
    }
    if (NULL != config->region) {
        sdsfree(config->region);
    }
    if (NULL != config->account_id) {
        sdsfree(config->account_id);
    }
    if (NULL != config->secret_key) {
        sdsfree(config->secret_key);
    }
    httpdns_list_free(&config->ipv4_boot_servers, (data_free_function_ptr_t) sdsfree);
    httpdns_list_free(&config->ipv6_boot_servers, (data_free_function_ptr_t) sdsfree);
    httpdns_list_free(&config->pre_resolve_hosts, (data_free_function_ptr_t) sdsfree);
    free(config);
}

