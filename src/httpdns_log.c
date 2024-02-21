//
// Created by caogaoshuai on 2024/2/1.
//
#include<stdio.h>
#include "httpdns_log.h"
#include "httpdns_sds.h"


static volatile FILE *log_file = NULL;

void httpdns_log_start() {
    if (NULL != log_file) {
        return;
    }
    log_set_level(HTTPDNS_LOG_LEVEL);
    log_set_quiet(true);
    char log_file_path[1024] = HTTPDNS_MICRO_TO_STRING(HTTPDNS_LOG_FILE_PATH);
    if (strlen(log_file_path) <= 0) {
        log_info("log file path is not set");
    }
    log_file = fopen(log_file_path, "ab+");
    if (NULL == log_file) {
        log_error("open log file failed, file path %s", log_file_path);
    } else {
        log_info("open log file success, file path %s", log_file_path);
        log_add_fp(log_file, LOG_TRACE);
    }
}


void httpdns_log_stop() {
    if (NULL != log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}
