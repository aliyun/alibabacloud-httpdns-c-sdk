//
// Created by cagaoshuai on 2024/1/15.
//

#include "httpdns_config.h"

static int32_t test_config_valid() {
    httpdns_config_t *config = create_httpdns_config();
    int32_t ret = httpdns_config_is_valid(config);
    if (ret == HTTPDNS_SUCCESS) {
        destroy_httpdns_config(config);
        return HTTPDNS_FAILURE;
    }
    httpdns_config_set_account_id(config, "139450");
    ret = httpdns_config_is_valid(config);
    if (ret == HTTPDNS_FAILURE) {
        destroy_httpdns_config(config);
        return HTTPDNS_FAILURE;
    }
    destroy_httpdns_config(config);
    return HTTPDNS_SUCCESS;
}

static int32_t test_add_pre_resolve_host() {
    httpdns_config_t *config = create_httpdns_config();
    httpdns_config_set_account_id(config, "139450");
    int32_t ret = httpdns_config_add_pre_resolve_host(config, "www.baidu.com");
    if (ret != HTTPDNS_SUCCESS) {
        destroy_httpdns_config(config);
        return HTTPDNS_FAILURE;
    }
    ret = httpdns_config_add_pre_resolve_host(config, "www.baidu.com");
    if (ret != HTTPDNS_LIST_NODE_DUPLICATED) {
        destroy_httpdns_config(config);
        return HTTPDNS_FAILURE;
    }
    destroy_httpdns_config(config);
    return HTTPDNS_SUCCESS;
}


int main(void) {
    if (test_config_valid() != HTTPDNS_SUCCESS) {
        return -1;
    }
    if (test_add_pre_resolve_host() != HTTPDNS_SUCCESS) {
        return -1;
    }
    return 0;
}
