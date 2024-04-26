//
// Created by cagaoshuai on 2024/4/19.
//

#ifndef HDNS_C_SDK_HDNS_LOCALDNS_H
#define HDNS_C_SDK_HDNS_LOCALDNS_H

#include "hdns_resolver.h"
#include "hdns_define.h"

HDNS_CPP_START

hdns_resv_resp_t *hdns_localdns_resolve(hdns_pool_t *pool, const char *host, hdns_rr_type_t type);

HDNS_CPP_END

#endif
