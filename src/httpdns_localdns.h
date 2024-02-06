//
// Created by caogaoshuai on 2024/2/5.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_LOCALDNS_H
#define HTTPDNS_C_SDK_HTTPDNS_LOCALDNS_H

#include "httpdns_resolve_result.h"

/**
 *    must free using httpdns_resolve_result_free
 */
httpdns_resolve_result_t *resolve_host_by_localdns(const char *host);

#endif //HTTPDNS_C_SDK_HTTPDNS_LOCALDNS_H
