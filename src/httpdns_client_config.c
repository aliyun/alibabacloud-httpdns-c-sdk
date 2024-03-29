//
// Created by caogaoshuai on 2024/1/10.
//

#include <stdlib.h>

#include "httpdns_list.h"
#include "httpdns_log.h"
#include "httpdns_memory.h"
#include "httpdns_sds.h"

#include "httpdns_client_config.h"


static void set_default_httpdns_config(httpdns_config_t *config) {
    config->using_cache = true;
    config->using_https = true;
    config->using_sign = false;
    config->timeout_ms = HTTPDNS_DEFAULT_TIMEOUT_MS;
#ifdef HTTPDNS_RETRY_TIMES
    config->retry_times = HTTPDNS_RETRY_TIMES;
#else
    config->retry_times = HTTPDNS_DEFAULT_RETRY_TIMES;
#endif
    config->sdk_version = httpdns_sds_new(HTTPDNS_SDK_VERSION);
    config->user_agent = httpdns_sds_new(HTTPDNS_USER_AGENT);
    httpdns_list_init(&config->pre_resolve_hosts);
    httpdns_list_init(&config->ipv4_boot_servers);
    httpdns_list_init(&config->ipv6_boot_servers);

    char httpdns_region[10] = HTTPDNS_MICRO_TO_STRING(HTTPDNS_REGION);
    config->region = httpdns_sds_new(httpdns_region);

    if (strcmp(httpdns_region, HTTPDNS_REGION_SINGAPORE) == 0) {
        httpdns_config_add_ipv4_boot_server(config, "161.117.200.122");
        httpdns_config_add_ipv4_boot_server(config, "8.219.89.41");
        httpdns_config_add_ipv6_boot_server(config, "240b:4000:f10::208");
    } else if (strcmp(httpdns_region, HTTPDNS_REGION_HONG_KONG) == 0) {
        httpdns_config_add_ipv4_boot_server(config, "47.56.234.194");
        httpdns_config_add_ipv6_boot_server(config, "240b:4000:f10::208");
    } else {
        httpdns_config_add_ipv4_boot_server(config, HTTPDNS_DEFAULT_IPV4_BOOT_SERVER);
        httpdns_config_add_ipv4_boot_server(config, "203.107.1.33");
        httpdns_config_add_ipv4_boot_server(config, "203.107.1.66");
        httpdns_config_add_ipv4_boot_server(config, "203.107.1.98");

        httpdns_config_add_ipv6_boot_server(config, HTTPDNS_DEFAULT_IPV6_BOOT_SERVER);
    }
    // 设置默认调度入口
    httpdns_config_add_ipv4_boot_server(config, HTTPDNS_DEFAULT_IPV4_BOOT_SERVER);
    httpdns_config_add_ipv4_boot_server(config, "httpdns-sc.aliyuncs.com");

    httpdns_config_add_ipv6_boot_server(config, HTTPDNS_DEFAULT_IPV6_BOOT_SERVER);
    httpdns_config_add_ipv6_boot_server(config, "httpdns-sc.aliyuncs.com");
}

httpdns_config_t *httpdns_config_new() {
    httpdns_new_object_in_heap(config, httpdns_config_t);
    set_default_httpdns_config(config);
    return config;
}


int32_t httpdns_config_set_account_id(httpdns_config_t *config, const char *account_id) {
    httpdns_set_string_field(config, account_id, account_id);
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_set_secret_key(httpdns_config_t *config, const char *secret_key) {
    httpdns_set_string_field(config, secret_key, secret_key);
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_set_net_probe_domain(httpdns_config_t *config, const char *probe_domain) {
    httpdns_set_string_field(config, probe_domain, probe_domain);
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_set_timeout_ms(httpdns_config_t *config, int32_t timeout_ms) {
    if (config == NULL || timeout_ms <= 0) {
        httpdns_log_info("httpdns config set timeout failed, config or timeout_ms param is null");
        return HTTPDNS_PARAMETER_ERROR;
    }
    config->timeout_ms = timeout_ms;
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_set_using_cache(httpdns_config_t *config, bool using_cache) {
    if (NULL == config) {
        httpdns_log_info("httpdns config set using cache failed, config is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->using_cache = using_cache;
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_set_using_https(httpdns_config_t *config, bool using_https) {
    if (NULL == config) {
        httpdns_log_info("httpdns config set using https failed, config is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->using_https = using_https;
    return HTTPDNS_SUCCESS;
}


int32_t httpdns_config_set_using_sign(httpdns_config_t *config, bool using_sign) {
    if (NULL == config) {
        httpdns_log_info("httpdns config set using sign failed, config is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    config->using_sign = using_sign;
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_add_pre_resolve_host(httpdns_config_t *config, const char *host) {
    if (NULL == config || NULL == host) {
        httpdns_log_info("httpdns config set pre-resolve host failed, config or host param is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (!httpdns_list_contain(&config->pre_resolve_hosts, host, httpdns_string_cmp_func)) {
        httpdns_list_add(&config->pre_resolve_hosts, host, httpdns_string_clone_func);
        return HTTPDNS_SUCCESS;
    }
    httpdns_log_info("httpdns config set pre-resolve host failed, host %s have been exist", host);
    return HTTPDNS_LIST_NODE_DUPLICATED;
}


int32_t httpdns_config_add_ipv4_boot_server(httpdns_config_t *config, const char *boot_server) {
    if (NULL == config || NULL == boot_server) {
        httpdns_log_info("httpdns config set ipv4 boot server failed, config or boot_server param is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (!httpdns_list_contain(&config->ipv4_boot_servers, boot_server, httpdns_string_cmp_func)) {
        httpdns_list_add(&config->ipv4_boot_servers, boot_server, httpdns_string_clone_func);
        return HTTPDNS_SUCCESS;
    }
    httpdns_log_info("httpdns config set ipv4 boot server failed, boot_server %s have been exist", boot_server);
    return HTTPDNS_LIST_NODE_DUPLICATED;
}

int32_t httpdns_config_add_ipv6_boot_server(httpdns_config_t *config, const char *boot_server) {
    if (NULL == config || NULL == boot_server) {
        httpdns_log_info("httpdns config set ipv6 boot server failed, config or boot_server param is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (!httpdns_list_contain(&config->ipv6_boot_servers, boot_server, httpdns_string_cmp_func)) {
        httpdns_list_add(&config->ipv6_boot_servers, boot_server, httpdns_string_clone_func);
        return HTTPDNS_SUCCESS;
    }
    httpdns_log_info("httpdns config set ipv6 boot server failed, boot_server %s have been exist", boot_server);
    return HTTPDNS_LIST_NODE_DUPLICATED;
}


int32_t httpdns_config_set_retry_times(httpdns_config_t *config, int32_t retry_times) {
    if (config == NULL || retry_times <= 0) {
        httpdns_log_info("httpdns config set timeout failed, config or timeout_ms param is null");
        return HTTPDNS_PARAMETER_ERROR;
    }
    config->retry_times = retry_times;
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_config_valid(httpdns_config_t *config) {
    if (NULL == config) {
        httpdns_log_info("httpdns config valid failed, config is null");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    if (httpdns_string_is_blank(config->account_id)) {
        httpdns_log_info("httpdns config valid failed, account_id is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (config->using_sign && httpdns_string_is_blank(config->secret_key)) {
        httpdns_log_info("httpdns config valid failed, using sign but secret_key is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (httpdns_list_is_empty(&config->ipv4_boot_servers) && httpdns_list_is_empty(&config->ipv6_boot_servers)) {
        httpdns_log_info("httpdns config valid failed, ipv4 boot servers and ipv6 boot servers is empty");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (httpdns_string_is_blank(config->region)) {
        httpdns_log_info("httpdns config valid failed, region is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (config->timeout_ms <= 0) {
        httpdns_log_info("httpdns config valid failed, timeout is not more than 0");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (httpdns_string_is_blank(config->sdk_version)) {
        httpdns_log_info("httpdns config valid failed, sdk_version is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    if (httpdns_string_is_blank(config->user_agent)) {
        httpdns_log_info("httpdns config valid failed, user agent is blank");
        return HTTPDNS_PARAMETER_ERROR;
    }
    return HTTPDNS_SUCCESS;
}

void httpdns_config_free(httpdns_config_t *config) {
    if (NULL == config) {
        return;
    }
    if (NULL != config->sdk_version) {
        httpdns_sds_free(config->sdk_version);
    }
    if (NULL != config->region) {
        httpdns_sds_free(config->region);
    }
    if (NULL != config->account_id) {
        httpdns_sds_free(config->account_id);
    }
    if (NULL != config->secret_key) {
        httpdns_sds_free(config->secret_key);
    }
    if (NULL != config->probe_domain) {
        httpdns_sds_free(config->probe_domain);
    }
    if (NULL != config->user_agent) {
        httpdns_sds_free(config->user_agent);
    }
    httpdns_list_free(&config->ipv4_boot_servers, (httpdns_data_free_func_t) httpdns_sds_free);
    httpdns_list_free(&config->ipv6_boot_servers, (httpdns_data_free_func_t) httpdns_sds_free);
    httpdns_list_free(&config->pre_resolve_hosts, (httpdns_data_free_func_t) httpdns_sds_free);
    free(config);
}

