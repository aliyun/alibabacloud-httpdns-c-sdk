//
// Created by caogaoshuai on 2024/1/14.
//

#include "httpdns_global_config.h"
#include <curl/curl.h>
#include <string.h>
#include "httpdns_log.h"

static volatile bool is_initialized = false;

void init_httpdns_sdk() {
    if (is_initialized) {
        log_info("httpdns sdk has been initialized, skip");
        return;
    }
    srand(time(NULL));
    curl_global_init(CURL_GLOBAL_ALL);
    curl_version_info_data *info = curl_version_info(CURLVERSION_NOW);
    if (info) {
        log_info("SSL INFO: %s\n", info->ssl_version);
    }
    is_initialized = true;
}


void cleanup_httpdns_sdk() {
    if (is_initialized) {
        curl_global_cleanup();
        is_initialized = false;
    }
}