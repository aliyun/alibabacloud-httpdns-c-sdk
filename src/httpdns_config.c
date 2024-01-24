//
// Created by cagaoshuai on 2024/1/10.
//


#include "httpdns_config.h"
#include "httpdns_list.h"
#include "httpdns_memory.h"
#include "sds.h"
#include <string.h>
#include <stdlib.h>


static void set_default_httpdns_config(httpdns_config_t *config_ptr) {
    config_ptr->using_async = true;
    config_ptr->using_cache = true;
    config_ptr->using_https = false;
    config_ptr->using_sign = false;
    config_ptr->fallbacking_localdns = true;
    config_ptr->timeout_ms = DEFAULT_TIMEOUT_MS;
    config_ptr->region = sdsnew(REGION_CHINA_MAINLAND);
    config_ptr->sdk_version = sdsnew(SDK_VERSION);
    config_ptr->user_agent = sdsnew(USER_AGENT);

    httpdns_list_init(&config_ptr->pre_resolve_hosts);
    httpdns_list_init(&config_ptr->ipv4_boot_servers);
    httpdns_list_add(&config_ptr->ipv4_boot_servers, DEFAULT_IPV4_BOOT_SERVER, STRING_CLONE_FUNC);
    httpdns_list_init(&config_ptr->ipv6_boot_servers);
    httpdns_list_add(&config_ptr->ipv6_boot_servers, DEFAULT_IPV6_BOOT_SERVER, STRING_CLONE_FUNC);

}

httpdns_config_t *create_httpdns_config() {
    httpdns_config_t *config_ptr = (httpdns_config_t *) malloc(sizeof(httpdns_config_t));
    memset(config_ptr, 0, sizeof(httpdns_config_t));
    set_default_httpdns_config(config_ptr);
    return config_ptr;
}


int32_t httpdns_config_set_account_id(httpdns_config_t *config, const char *account_id) {
    HTTPDNS_SET_STRING_FIELD(config, account_id, account_id);
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_set_secret_key(httpdns_config_t *config, const char *secret_key) {
    HTTPDNS_SET_STRING_FIELD(config, secret_key, secret_key);
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_set_net_probe_domain(httpdns_config_t *config, const char *probe_domain) {
    HTTPDNS_SET_STRING_FIELD(config, probe_domain, probe_domain);
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_set_timeout_ms(httpdns_config_t *config, int32_t timeout_ms) {
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
    if (!httpdns_list_contain(&config->pre_resolve_hosts, host, STRING_CMP_FUNC)) {
        httpdns_list_add(&config->pre_resolve_hosts, host, STRING_CLONE_FUNC);
        return HTTPDNS_SUCCESS;
    }
    return HTTPDNS_LIST_NODE_DUPLICATED;
}


int32_t httpdns_config_add_ipv4_boot_server(httpdns_config_t *config, const char *boot_server) {
    if (NULL == config || NULL == boot_server) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (!httpdns_list_contain(&config->ipv4_boot_servers, boot_server, STRING_CMP_FUNC)) {
        httpdns_list_add(&config->ipv4_boot_servers, boot_server, STRING_CLONE_FUNC);
        return HTTPDNS_SUCCESS;
    }
    return HTTPDNS_LIST_NODE_DUPLICATED;
}

int32_t httpdns_config_add_ipv6_boot_server(httpdns_config_t *config, const char *boot_server) {
    if (NULL == config || NULL == boot_server) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (!httpdns_list_contain(&config->ipv6_boot_servers, boot_server, STRING_CMP_FUNC)) {
        httpdns_list_add(&config->ipv6_boot_servers, boot_server, STRING_CLONE_FUNC);
        return HTTPDNS_SUCCESS;
    }
    return HTTPDNS_LIST_NODE_DUPLICATED;
}

int32_t httpdns_config_is_valid(httpdns_config_t *config) {
    if (NULL == config) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (IS_BLANK_STRING(config->account_id)) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (config->using_sign && IS_BLANK_STRING(config->secret_key)) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (IS_EMPTY_LIST(&config->ipv4_boot_servers) && IS_EMPTY_LIST(&config->ipv6_boot_servers)) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (IS_BLANK_STRING(config->region)) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (config->timeout_ms <= 0) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (IS_BLANK_STRING(config->sdk_version)) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (IS_BLANK_STRING(config->user_agent)) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    return HTTPDNS_SUCCESS;
}

void destroy_httpdns_config(httpdns_config_t *config) {
    if (NULL == config) {
        return;
    }
    if (NULL != config->sdk_version) {
        sdsfree(config->sdk_version);
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
    if (NULL != config->probe_domain) {
        sdsfree(config->probe_domain);
    }
    if(NULL != config->user_agent) {
        sdsfree(config->user_agent);
    }
    httpdns_list_free(&config->ipv4_boot_servers, (data_free_function_ptr_t) sdsfree);
    httpdns_list_free(&config->ipv6_boot_servers, (data_free_function_ptr_t) sdsfree);
    httpdns_list_free(&config->pre_resolve_hosts, (data_free_function_ptr_t) sdsfree);
    free(config);
}

