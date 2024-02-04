//
// Created by caogaoshuai on 2024/1/10.
//


#include "httpdns_client_config.h"
#include "httpdns_list.h"
#include "httpdns_string.h"
#include "httpdns_memory.h"
#include "sds.h"
#include <string.h>
#include <stdlib.h>
#include "log.h"


static void set_default_httpdns_config(httpdns_config_t *config) {
    config->using_async = true;
    config->using_cache = true;
    config->using_https = false;
    config->using_sign = false;
    config->fallbacking_localdns = true;
    config->timeout_ms = DEFAULT_TIMEOUT_MS;
#ifdef HTTPDNS_RETRY_TIMES
    config->retry_times = HTTPDNS_RETRY_TIMES;
#else
    config->retry_times = DEFAULT_RETRY_TIMES;
#endif
    config->sdk_version = sdsnew(SDK_VERSION);
    config->user_agent = sdsnew(USER_AGENT);
    httpdns_list_init(&config->pre_resolve_hosts);
    httpdns_list_init(&config->ipv4_boot_servers);
    httpdns_list_init(&config->ipv6_boot_servers);

    char httpdns_region[10] = MICRO_TO_STRING(HTTPDNS_REGION);
    config->region = sdsnew(httpdns_region);

    if (strcmp(httpdns_region, REGION_SINGAPORE) == 0) {
        httpdns_config_add_ipv4_boot_server(config, "161.117.200.122");
        httpdns_config_add_ipv4_boot_server(config, "8.219.89.41");
        httpdns_config_add_ipv6_boot_server(config, "240b:4000:f10::208");
    } else if (strcmp(httpdns_region, REGION_HONG_KONG) == 0) {
        httpdns_config_add_ipv4_boot_server(config, "47.56.234.194");
        httpdns_config_add_ipv6_boot_server(config, "240b:4000:f10::208");
    } else {
        httpdns_config_add_ipv4_boot_server(config, DEFAULT_IPV4_BOOT_SERVER);
        httpdns_config_add_ipv4_boot_server(config, "203.107.1.33");
        httpdns_config_add_ipv4_boot_server(config, "203.107.1.66");
        httpdns_config_add_ipv4_boot_server(config, "203.107.1.98");

        httpdns_config_add_ipv6_boot_server(config, DEFAULT_IPV6_BOOT_SERVER);
    }
    // 设置默认调度入口
    httpdns_config_add_ipv4_boot_server(config, DEFAULT_IPV4_BOOT_SERVER);
    httpdns_config_add_ipv4_boot_server(config, "httpdns-sc.aliyuncs.com");

    httpdns_config_add_ipv6_boot_server(config, DEFAULT_IPV6_BOOT_SERVER);
    httpdns_config_add_ipv6_boot_server(config, "httpdns-sc.aliyuncs.com");
}

httpdns_config_t *httpdns_config_new() {
    HTTPDNS_NEW_OBJECT_IN_HEAP(config, httpdns_config_t);
    set_default_httpdns_config(config);
    return config;
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
        log_info("httpdns config set timeout failed, config or timeout_ms param is null");
        return HTTPDNS_PARAMETER_ERROR;
    }
    config->timeout_ms = timeout_ms;
    return HTTPDNS_SUCCESS;
}


int32_t httpdns_config_set_using_async(httpdns_config_t *config, bool using_async) {
    if (NULL == config) {
        log_info("httpdns config set async failed, config is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->using_async = using_async;
    return HTTPDNS_SUCCESS;
}


int32_t httpdns_config_set_using_cache(httpdns_config_t *config, bool using_cache) {
    if (NULL == config) {
        log_info("httpdns config set using cache failed, config is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->using_cache = using_cache;
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_set_using_https(httpdns_config_t *config, bool using_https) {
    if (NULL == config) {
        log_info("httpdns config set using https failed, config is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->using_https = using_https;
    return HTTPDNS_SUCCESS;
}


int32_t httpdns_config_set_using_sign(httpdns_config_t *config, bool using_sign) {
    if (NULL == config) {
        log_info("httpdns config set using sign failed, config is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->using_sign = using_sign;
    return HTTPDNS_SUCCESS;
}


int32_t httpdns_config_set_fallbacking_localdns(httpdns_config_t *config, bool fallbacking_localdns) {
    if (NULL == config) {
        log_info("httpdns config set fallbacking localdns failed, config is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->fallbacking_localdns = fallbacking_localdns;
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_add_pre_resolve_host(httpdns_config_t *config, const char *host) {
    if (NULL == config || NULL == host) {
        log_info("httpdns config set pre-resolve host failed, config or host param is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (!httpdns_list_contain(&config->pre_resolve_hosts, host, STRING_CMP_FUNC)) {
        httpdns_list_add(&config->pre_resolve_hosts, host, STRING_CLONE_FUNC);
        return HTTPDNS_SUCCESS;
    }
    log_info("httpdns config set pre-resolve host failed, host %s have been exist", host);
    return HTTPDNS_LIST_NODE_DUPLICATED;
}


int32_t httpdns_config_add_ipv4_boot_server(httpdns_config_t *config, const char *boot_server) {
    if (NULL == config || NULL == boot_server) {
        log_info("httpdns config set ipv4 boot server failed, config or boot_server param is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (!httpdns_list_contain(&config->ipv4_boot_servers, boot_server, STRING_CMP_FUNC)) {
        httpdns_list_add(&config->ipv4_boot_servers, boot_server, STRING_CLONE_FUNC);
        return HTTPDNS_SUCCESS;
    }
    log_info("httpdns config set ipv4 boot server failed, boot_server %s have been exist", boot_server);
    return HTTPDNS_LIST_NODE_DUPLICATED;
}

int32_t httpdns_config_add_ipv6_boot_server(httpdns_config_t *config, const char *boot_server) {
    if (NULL == config || NULL == boot_server) {
        log_info("httpdns config set ipv6 boot server failed, config or boot_server param is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (!httpdns_list_contain(&config->ipv6_boot_servers, boot_server, STRING_CMP_FUNC)) {
        httpdns_list_add(&config->ipv6_boot_servers, boot_server, STRING_CLONE_FUNC);
        return HTTPDNS_SUCCESS;
    }
    log_info("httpdns config set ipv6 boot server failed, boot_server %s have been exist", boot_server);
    return HTTPDNS_LIST_NODE_DUPLICATED;
}


int32_t httpdns_config_set_retry_times(httpdns_config_t *config, int32_t retry_times) {
    if (config == NULL || retry_times <= 0) {
        log_info("httpdns config set timeout failed, config or timeout_ms param is null");
        return HTTPDNS_PARAMETER_ERROR;
    }
    config->retry_times = retry_times;
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_valid(httpdns_config_t *config) {
    if (NULL == config) {
        log_info("httpdns config valid failed, config is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (IS_BLANK_STRING(config->account_id)) {
        log_info("httpdns config valid failed, account_id is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (config->using_sign && IS_BLANK_STRING(config->secret_key)) {
        log_info("httpdns config valid failed, using sign but secret_key is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (IS_EMPTY_LIST(&config->ipv4_boot_servers) && IS_EMPTY_LIST(&config->ipv6_boot_servers)) {
        log_info("httpdns config valid failed, ipv4 boot servers and ipv6 boot servers is empty");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (IS_BLANK_STRING(config->region)) {
        log_info("httpdns config valid failed, region is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (config->timeout_ms <= 0) {
        log_info("httpdns config valid failed, timeout is not more than 0");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (IS_BLANK_STRING(config->sdk_version)) {
        log_info("httpdns config valid failed, sdk_version is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (IS_BLANK_STRING(config->user_agent)) {
        log_info("httpdns config valid failed, user agent is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    return HTTPDNS_SUCCESS;
}

void httpdns_config_free(httpdns_config_t *config) {
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
    if (NULL != config->user_agent) {
        sdsfree(config->user_agent);
    }
    httpdns_list_free(&config->ipv4_boot_servers, (data_free_function_ptr_t) sdsfree);
    httpdns_list_free(&config->ipv6_boot_servers, (data_free_function_ptr_t) sdsfree);
    httpdns_list_free(&config->pre_resolve_hosts, (data_free_function_ptr_t) sdsfree);
    free(config);
}

