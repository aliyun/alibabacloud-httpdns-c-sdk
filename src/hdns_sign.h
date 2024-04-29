//
// Created by caogaoshuai on 2024/1/19.
//

#ifndef HDNS_C_SDK_HDNS_SIGN_H
#define HDNS_C_SDK_HDNS_SIGN_H

#include "hdns_define.h"

HDNS_CPP_START

#define HDNS_MAX_RESOLVE_SIGN_OFFSET_TIME  (30 * 60)
#define HDNS_MAX_SCHEDULE_SIGN_OFFSET_TIME  0

typedef struct {
    hdns_pool_t *pool;
    char *raw;
    char *sign;
    char *timestamp;
} hdns_sign_t;


hdns_sign_t *hdns_gen_resv_req_sign(hdns_pool_t *pool, const char *host, const char *secret);

hdns_sign_t *hdns_gen_sched_req_sign(hdns_pool_t *pool, const char *nonce, const char *secret);

HDNS_CPP_END

#endif
