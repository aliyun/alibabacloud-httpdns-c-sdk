//
// Created by cagaoshuai on 2024/3/26.
//

#ifndef HDNS_C_SDK_HDNS_SESSION_H
#define HDNS_C_SDK_HDNS_SESSION_H

int hdns_session_pool_init(hdns_pool_t *parent_pool, int flags);

CURL *hdns_session_require();

void hdns_session_release(CURL *session);

void hdns_session_pool_cleanup();

#endif
