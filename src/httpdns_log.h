//
// Created by caogaoshuai on 2024/2/1.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_LOG_H
#define HTTPDNS_C_SDK_HTTPDNS_LOG_H

#include "log.h"
#include "configuration.h"
/**
 * must free using httpdns_log_stop
 */
void httpdns_log_start();

void httpdns_log_stop();

#endif //HTTPDNS_C_SDK_HTTPDNS_LOG_H
