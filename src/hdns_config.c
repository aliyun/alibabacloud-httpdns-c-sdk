//
// Created by caogaoshuai on 2024/1/10.
//
#include "hdns_list.h"
#include "hdns_define.h"
#include "hdns_status.h"

#include "hdns_config.h"

#define HTTPDNS_REGION_CHINA_MAINLAND      "cn"
#define HTTPDNS_REGION_HONG_KONG           "hk"
#define HTTPDNS_REGION_SINGAPORE           "sg"


static void generate_session_id(char *buff, size_t len) {
    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
    size_t alphanum_size = sizeof(alphanum) - 1;
    for (int i = 0; i < len; i++) {
        buff[i] = alphanum[rand() % alphanum_size];
    }
    buff[len] = '\0';
}


static void set_default_hdns_config(hdns_pool_t *pool, hdns_config_t *config) {
    config->pool = pool;
    config->account_id = NULL;
    config->secret_key = NULL;
    config->timeout = HDNS_MAX_TIMEOUT_MS;
    config->retry_times = 1;

    config->using_cache = true;
    config->using_https = true;
    config->using_sign = false;
    config->enable_expired_ip = false;
    config->enable_failover_localdns = false;

    char session_id[HDNS_SID_STRING_LEN + 1];
    generate_session_id(session_id, HDNS_SID_STRING_LEN);
    config->session_id = apr_pstrdup(pool, session_id);

    config->pre_resolve_hosts = hdns_list_new(pool);
    config->ipv4_boot_servers = hdns_list_new(pool);
    config->ipv6_boot_servers = hdns_list_new(pool);

#ifdef HTTPDNS_REGION
    config->region = apr_pstrdup(pool, HTTPDNS_REGION);
    if (strcmp(HTTPDNS_REGION, HTTPDNS_REGION_SINGAPORE) == 0) {
        hdns_list_add(config->ipv4_boot_servers, "161.117.200.122", NULL);
        hdns_list_add(config->ipv4_boot_servers, "47.74.222.190", NULL);
        hdns_list_add(config->ipv4_boot_servers, "203.107.1.97", NULL);
        hdns_list_add(config->ipv4_boot_servers, "httpdns-sc.aliyuncs.com", NULL);

        hdns_list_add(config->ipv6_boot_servers, "240b:4000:f10::178", NULL);
        hdns_list_add(config->ipv6_boot_servers, "240b:4000:f10::188", NULL);
    } else if (strcmp(HTTPDNS_REGION, HTTPDNS_REGION_HONG_KONG) == 0) {

        hdns_list_add(config->ipv4_boot_servers, "47.56.234.194", NULL);
        hdns_list_add(config->ipv4_boot_servers, "47.56.119.115", NULL);
        hdns_list_add(config->ipv4_boot_servers, "203.107.1.97", NULL);
        hdns_list_add(config->ipv4_boot_servers, "httpdns-sc.aliyuncs.com", NULL);

        hdns_list_add(config->ipv6_boot_servers, "240b:4000:f10::178", NULL);
        hdns_list_add(config->ipv6_boot_servers, "240b:4000:f10::188", NULL);
    } else {
        config->region = apr_pstrdup(pool, HTTPDNS_REGION_CHINA_MAINLAND);
        hdns_list_add(config->ipv4_boot_servers, "203.107.1.1", NULL);
        hdns_list_add(config->ipv4_boot_servers, "203.107.1.97", NULL);
        hdns_list_add(config->ipv4_boot_servers, "203.107.1.100", NULL);
        hdns_list_add(config->ipv4_boot_servers, "httpdns-sc.aliyuncs.com", NULL);

        hdns_list_add(config->ipv6_boot_servers, "2401:b180:2000:30::1c", NULL);
        hdns_list_add(config->ipv6_boot_servers, "2401:b180:2000:20::10", NULL);
    }
#else
    config->region = apr_pstrdup(pool, HTTPDNS_REGION_CHINA_MAINLAND);
    hdns_list_add(config->ipv4_boot_servers, "203.107.1.1", NULL);
    hdns_list_add(config->ipv4_boot_servers, "203.107.1.97", NULL);
    hdns_list_add(config->ipv4_boot_servers, "203.107.1.100", NULL);
    hdns_list_add(config->ipv4_boot_servers, "httpdns-sc.aliyuncs.com", NULL);

    hdns_list_add(config->ipv6_boot_servers, "2401:b180:2000:30::1c", NULL);
    hdns_list_add(config->ipv6_boot_servers, "2401:b180:2000:20::10", NULL);
#endif

    config->ip_probe_items = apr_hash_make(pool);
    config->custom_ttl_items = apr_hash_make(pool);
}

hdns_config_t *hdns_config_create() {
    hdns_pool_new(pool);
    hdns_config_t *config = hdns_palloc(pool, sizeof(hdns_config_t));
    set_default_hdns_config(pool, config);
    apr_thread_mutex_create(&config->lock, APR_THREAD_MUTEX_DEFAULT, pool);
    return config;
}

hdns_status_t hdns_config_valid(hdns_config_t *config) {
    if (NULL == config) {
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "config is null",
                                 NULL);
    }
    apr_thread_mutex_lock(config->lock);
    if (hdns_str_is_blank(config->account_id)) {
        apr_thread_mutex_unlock(config->lock);
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "account_id is blank",
                                 config->session_id);
    }
    if (config->using_sign && hdns_str_is_blank(config->secret_key)) {
        apr_thread_mutex_unlock(config->lock);
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "using sign but secret_key is blank",
                                 config->session_id);
    }
    if (hdns_list_is_empty(config->ipv4_boot_servers) && hdns_list_is_empty(config->ipv6_boot_servers)) {
        apr_thread_mutex_unlock(config->lock);
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "ipv4 boot servers and ipv6 boot servers is empty",
                                 config->session_id);
    }
    if (hdns_str_is_blank(config->region)) {
        apr_thread_mutex_unlock(config->lock);
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "region is blank",
                                 config->session_id);
    }
    if (config->timeout <= 0) {
        apr_thread_mutex_unlock(config->lock);
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "timeout is less than 0",
                                 config->session_id);
    }
    apr_thread_mutex_unlock(config->lock);
    return hdns_status_ok(config->session_id);
}


void hdns_config_cleanup(hdns_config_t *config) {
    if (NULL == config) {
        return;
    }
    apr_thread_mutex_destroy(config->lock);
    hdns_pool_destroy(config->pool);
}
