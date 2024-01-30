//
// Created by cagaoshuai on 2024/1/14.
//
#include "httpdns_global_config.h"
#include <curl/curl.h>
#include "configuration.h"
#include "log.h"
#include "string.h"
#include "sds.h"

static FILE *log_file = NULL;


void init_log() {
    log_set_level(HTTPDNS_LOG_LEVEL);
    log_set_quiet(true);
    char log_file_path[1024] = MICRO_TO_STRING(LOG_FILE_PATH);
    if (strlen(log_file_path) <= 0) {
        log_info("log file path is not set");
    }
    log_file = fopen(log_file_path, "ab+");
    if (NULL == log_file) {
        log_error("open log file failed, file path %s", log_file_path);
    } else {
        log_info("open log file success, file path %s", log_file_path);
        log_add_fp(log_file, LOG_TRACE);
        log_add_fp(log_file, LOG_DEBUG);
        log_add_fp(log_file, LOG_INFO);
        log_add_fp(log_file, LOG_WARN);
        log_add_fp(log_file, LOG_ERROR);
        log_add_fp(log_file, LOG_FATAL);
    }
}

void init_httpdns_sdk() {
    init_log();
    srand(time(NULL));
    curl_global_init(CURL_GLOBAL_ALL);
    curl_version_info_data *info = curl_version_info(CURLVERSION_NOW);
    if (info) {
        log_info("SSL INFO: %s\n", info->ssl_version);
    }
}


void cleanup_httpdns_sdk() {
    curl_global_cleanup();
    if (NULL != log_file) {
        fclose(log_file);
    }
}