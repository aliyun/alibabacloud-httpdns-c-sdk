//
// Created by caogaoshuai on 2024/1/19.
//
#include "hdns_log.h"
#include "hdns_sign.h"
#include "hdns_utils.h"

static hdns_sign_t *hdns_generate_sign(hdns_pool_t *pool,
                                       const char *content,
                                       const char *secret,
                                       int32_t max_offset);

static hdns_sign_t *hdns_generate_sign(hdns_pool_t *pool,
                                       const char *content,
                                       const char *secret,
                                       int32_t max_offset) {
    if (NULL == content || NULL == secret) {
        return NULL;
    }
    if (pool == NULL) {
        hdns_pool_create(&pool, NULL);
    }
    apr_time_t now = apr_time_now() / APR_USEC_PER_SEC + max_offset;
    char time_str[64];
    apr_snprintf(time_str, sizeof(time_str), "%" APR_INT64_T_FMT, now);

    char *str_for_sign = apr_pstrcat(pool, content, "-", secret, "-", time_str, NULL);

    char hex_sign_str[HDNS_MD5_STRING_LEN + 1];

    hdns_md5(str_for_sign, strlen(str_for_sign), hex_sign_str);

    hdns_sign_t *signature = hdns_palloc(pool, sizeof(hdns_sign_t));
    signature->sign = apr_pstrdup(pool, hex_sign_str);
    signature->timestamp = apr_pstrdup(pool, time_str);
    signature->raw = str_for_sign;
    signature->pool = pool;
    hdns_log_debug("hdns_sign_t(raw=%s,timestamp=%s,sign=%s)",
                   signature->raw,
                   signature->timestamp,
                   signature->sign);
    return signature;
}


hdns_sign_t *hdns_gen_resv_req_sign(hdns_pool_t *pool, const char *host, const char *secret) {
    return hdns_generate_sign(pool, host, secret, HDNS_MAX_RESOLVE_SIGN_OFFSET_TIME);
}

hdns_sign_t *hdns_gen_sched_req_sign(hdns_pool_t *pool, const char *nonce, const char *secret) {
    return hdns_generate_sign(pool, nonce, secret, HDNS_MAX_SCHEDULE_SIGN_OFFSET_TIME);
}
