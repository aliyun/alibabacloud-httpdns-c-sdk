#ifndef HDNS_C_SDK_HDNS_HTTP_H
#define HDNS_C_SDK_HDNS_HTTP_H

#include "hdns_transport.h"

HDNS_CPP_START

hdns_http_controller_t *hdns_http_controller_create(hdns_pool_t *p);

hdns_http_request_t *hdns_http_request_create(hdns_pool_t *p);

hdns_http_response_t *hdns_http_response_create(hdns_pool_t *p);

bool hdns_http_should_retry(hdns_http_response_t *http_resp);

int hdns_http_send_request(hdns_http_controller_t *ctl, hdns_http_request_t *req, hdns_http_response_t *resp);

HDNS_CPP_END

#endif

