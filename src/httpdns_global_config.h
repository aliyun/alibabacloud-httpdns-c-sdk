//
// Created by caogaoshuai on 2024/1/14.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_GLOBLE_CONFIG_H
#define HTTPDNS_C_SDK_HTTPDNS_GLOBLE_CONFIG_H

#include <time.h>
#include <stdlib.h>
/**
 * must cleanup using cleanup_httpdns_sdk
 */
void init_httpdns_sdk();

void cleanup_httpdns_sdk();

#endif //HTTPDNS_C_SDK_HTTPDNS_GLOBLE_CONFIG_H
