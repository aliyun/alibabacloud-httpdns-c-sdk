//
// Created by cagaoshuai on 2024/1/14.
//
#include "httpdns_global_config.h"
#include <curl/curl.h>

void init_httpdns_sdk() {
    srand(time(NULL));
    curl_global_init(CURL_GLOBAL_ALL);
}

void cleanup_httpdns_sdk() {
    curl_global_cleanup();
}