//
// Created by cagaoshuai on 2024/1/14.
//
#include "httpdns_global_config.h"
#include <curl/curl.h>
#include "configuration.h"
#include "log.h"


void init_httpdns_sdk() {
    srand(time(NULL));
    curl_global_init(CURL_GLOBAL_ALL);
    // check openssl
    curl_version_info_data *info = curl_version_info(CURLVERSION_NOW);
    if (info) {
        printf("SSL INFO: %s\n", info->ssl_version);
    }
    log_set_level(HTTPDNS_LOG_LEVEL);
}

void cleanup_httpdns_sdk() {
    curl_global_cleanup();
}