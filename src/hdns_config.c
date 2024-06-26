//
// Created by caogaoshuai on 2024/1/10.
//
#include "hdns_list.h"
#include "hdns_define.h"
#include "hdns_status.h"

#include "hdns_config.h"

#define HTTPDNS_REGION_CHINA_MAINLAND      "cn"
#define HTTPDNS_REGION_GLOBAL              "global"


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

static void init_boot_servers(hdns_pool_t *pool, hdns_config_t *config) {
    config->ipv4_boot_servers = apr_hash_make(pool);
    config->ipv6_boot_servers = apr_hash_make(pool);

    // 中国大陆调度中心
    hdns_list_head_t *cn_ipv4_boot_servers = hdns_list_new(pool);
    hdns_list_head_t *cn_ipv6_boot_servers = hdns_list_new(pool);

    hdns_list_add(cn_ipv4_boot_servers, "203.107.1.1", NULL);
    hdns_list_add(cn_ipv4_boot_servers, "203.107.1.97", NULL);
    hdns_list_add(cn_ipv4_boot_servers, "203.107.1.100", NULL);
    hdns_list_add(cn_ipv4_boot_servers, "203.119.238.240", NULL);
    hdns_list_add(cn_ipv4_boot_servers, "106.11.25.239", NULL);
    hdns_list_add(cn_ipv4_boot_servers, "59.82.99.47", NULL);
    hdns_list_add(cn_ipv4_boot_servers, "resolvers-cn.httpdns.aliyuncs.com", NULL);

    hdns_list_add(cn_ipv6_boot_servers, "2401:b180:7001::31d", NULL);
    hdns_list_add(cn_ipv6_boot_servers, "2408:4003:1f40::30a", NULL);
    hdns_list_add(cn_ipv6_boot_servers, "2401:b180:2000:20::10", NULL);
    hdns_list_add(cn_ipv6_boot_servers, "2401:b180:2000:30::1c", NULL);
    hdns_list_add(cn_ipv6_boot_servers, "resolvers-cn.httpdns.aliyuncs.com", NULL);

    apr_hash_set(config->ipv4_boot_servers, HTTPDNS_REGION_CHINA_MAINLAND, APR_HASH_KEY_STRING, cn_ipv4_boot_servers);
    apr_hash_set(config->ipv6_boot_servers, HTTPDNS_REGION_CHINA_MAINLAND, APR_HASH_KEY_STRING, cn_ipv6_boot_servers);

    // 香港调度中心
    hdns_list_head_t *hk_ipv4_boot_servers = hdns_list_new(pool);
    hdns_list_head_t *hk_ipv6_boot_servers = hdns_list_new(pool);

    hdns_list_add(hk_ipv4_boot_servers, "47.56.234.194", NULL);
    hdns_list_add(hk_ipv4_boot_servers, "47.56.119.115", NULL);
    hdns_list_add(hk_ipv4_boot_servers, "resolvers-hk.httpdns.aliyuncs.com", NULL);

    hdns_list_add(hk_ipv6_boot_servers, "240b:4000:f10::178", NULL);
    hdns_list_add(hk_ipv6_boot_servers, "240b:4000:f10::188", NULL);
    hdns_list_add(hk_ipv6_boot_servers, "resolvers-hk.httpdns.aliyuncs.com", NULL);

    apr_hash_set(config->ipv4_boot_servers, "hk", APR_HASH_KEY_STRING, hk_ipv4_boot_servers);
    apr_hash_set(config->ipv6_boot_servers, "hk", APR_HASH_KEY_STRING, hk_ipv6_boot_servers);

    // 新加坡调度中心
    hdns_list_head_t *sg_ipv4_boot_servers = hdns_list_new(pool);
    hdns_list_head_t *sg_ipv6_boot_servers = hdns_list_new(pool);

    hdns_list_add(sg_ipv4_boot_servers, "161.117.200.122", NULL);
    hdns_list_add(sg_ipv4_boot_servers, "47.74.222.190", NULL);
    hdns_list_add(sg_ipv4_boot_servers, "resolvers-sg.httpdns.aliyuncs.com", NULL);

    hdns_list_add(sg_ipv6_boot_servers, "240b:4000:f10::178", NULL);
    hdns_list_add(sg_ipv6_boot_servers, "240b:4000:f10::188", NULL);
    hdns_list_add(sg_ipv6_boot_servers, "resolvers-sg.httpdns.aliyuncs.com", NULL);

    apr_hash_set(config->ipv4_boot_servers, "sg", APR_HASH_KEY_STRING, sg_ipv4_boot_servers);
    apr_hash_set(config->ipv6_boot_servers, "sg", APR_HASH_KEY_STRING, sg_ipv6_boot_servers);

    // 美国调度中心
    hdns_list_head_t *us_ipv4_boot_servers = hdns_list_new(pool);
    hdns_list_head_t *us_ipv6_boot_servers = hdns_list_new(pool);

    hdns_list_add(us_ipv4_boot_servers, "47.246.131.175", NULL);
    hdns_list_add(us_ipv4_boot_servers, "47.246.131.141", NULL);
    hdns_list_add(us_ipv4_boot_servers, "resolvers-us.httpdns.aliyuncs.com", NULL);

    hdns_list_add(us_ipv6_boot_servers, "2404:2280:4000::2bb", NULL);
    hdns_list_add(us_ipv6_boot_servers, "2404:2280:4000::23e", NULL);
    hdns_list_add(us_ipv6_boot_servers, "resolvers-us.httpdns.aliyuncs.com", NULL);

    apr_hash_set(config->ipv4_boot_servers, "us", APR_HASH_KEY_STRING, us_ipv4_boot_servers);
    apr_hash_set(config->ipv6_boot_servers, "us", APR_HASH_KEY_STRING, us_ipv6_boot_servers);

    // 德国调度中心
    hdns_list_head_t *de_ipv4_boot_servers = hdns_list_new(pool);
    hdns_list_head_t *de_ipv6_boot_servers = hdns_list_new(pool);

    hdns_list_add(de_ipv4_boot_servers, "47.89.80.182", NULL);
    hdns_list_add(de_ipv4_boot_servers, "47.246.146.77", NULL);
    hdns_list_add(de_ipv4_boot_servers, "resolvers-de.httpdns.aliyuncs.com", NULL);

    hdns_list_add(de_ipv6_boot_servers, "2404:2280:3000::176", NULL);
    hdns_list_add(de_ipv6_boot_servers, "2404:2280:3000::188", NULL);
    hdns_list_add(de_ipv6_boot_servers, "resolvers-de.httpdns.aliyuncs.com", NULL);

    apr_hash_set(config->ipv4_boot_servers, "de", APR_HASH_KEY_STRING, de_ipv4_boot_servers);
    apr_hash_set(config->ipv6_boot_servers, "de", APR_HASH_KEY_STRING, de_ipv6_boot_servers);
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
    config->ip_probe_items = apr_hash_make(pool);
    config->custom_ttl_items = apr_hash_make(pool);

    init_boot_servers(pool, config);
#ifdef HTTPDNS_REGION
    config->region = apr_pstrdup(pool, HTTPDNS_REGION);
#else
    config->region = apr_pstrdup(pool, HTTPDNS_REGION_GLOBAL);
#endif
    config->boot_server_region = apr_pstrdup(pool, HTTPDNS_REGION_CHINA_MAINLAND);
}

hdns_config_t *hdns_config_create() {
    hdns_pool_new(pool);
    hdns_config_t *config = hdns_palloc(pool, sizeof(hdns_config_t));
    set_default_hdns_config(pool, config);
    apr_thread_mutex_create(&config->lock, APR_THREAD_MUTEX_DEFAULT, pool);
    return config;
}

hdns_list_head_t *hdns_config_get_boot_servers(hdns_config_t *config, bool ipv4) {
    if (NULL == config) {
        return NULL;
    }
    apr_hash_t *boot_server_table = ipv4 ? config->ipv4_boot_servers : config->ipv6_boot_servers;
    hdns_list_head_t *boot_servers = apr_hash_get(boot_server_table, config->boot_server_region, APR_HASH_KEY_STRING);
    return hdns_list_is_empty(boot_servers)
           ? apr_hash_get(boot_server_table, HTTPDNS_REGION_CHINA_MAINLAND, APR_HASH_KEY_STRING)
           : boot_servers;
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
    if (hdns_str_is_blank(config->region)) {
        apr_thread_mutex_unlock(config->lock);
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "region is blank",
                                 config->session_id);
    }
    if (hdns_str_is_blank(config->boot_server_region)) {
        apr_thread_mutex_unlock(config->lock);
        return hdns_status_error(HDNS_FAILED_VERIFICATION,
                                 HDNS_FAILED_VERIFICATION_CODE,
                                 "boot_server_region is blank",
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
