//
// 把HTTPDNS解析的相关参数转化为HTTPDNS请求，并通过回调透出HTTPDNS响应结果
//
// Created by caogaoshuai on 2024/1/9.
//

#ifndef HDNS_C_SDK_HDNS_RESOLVER_H
#define HDNS_C_SDK_HDNS_RESOLVER_H


#include "hdns_http.h"
#include "hdns_config.h"
#include "hdns_scheduler.h"
#include "hdns_status.h"

#include "hdns_define.h"

HDNS_CPP_START

#define  HDNS_QUERY_TYPE_A        "4"
#define  HDNS_QUERY_TYPE_AAAA     "6"
#define  HDNS_QUERY_TYPE_BOTH   "4,6"


typedef enum {
    HDNS_RR_TYPE_A = 1,
    HDNS_RR_TYPE_AAAA = 28
} hdns_rr_type_t;

typedef struct {
    hdns_pool_t *pool;
    char *host;
    char *client_ip;
    char *extra;
    hdns_list_head_t *ips;
    hdns_rr_type_t type;
    int origin_ttl;
    int ttl;
    int64_t query_time;
    char *cache_key;
    bool from_localdns;
} hdns_resv_resp_t;

typedef void (*hdns_resv_resp_cb_fn_t)(const hdns_resv_resp_t *resp, void *param);

typedef struct {
    hdns_pool_t *pool;
    char *host;
    char *account_id;
    char *secret_key;
    char *resolver;
    hdns_query_type_t query_type;
    char *client_ip;
    char *session_id;
    char *user_agent;
    hdns_table_t *sdns_params;
    bool using_https;
    bool using_sign;
    bool using_multi;
    bool using_cache;
    int32_t timeout_ms;
    int32_t retry_times;
    char *cache_key;
    hdns_resv_resp_cb_fn_t resv_resp_callback;
    void *resv_resp_cb_param;
    apr_thread_mutex_t *lock;
} hdns_resv_req_t;


hdns_resv_resp_t *hdns_resv_resp_create_empty(hdns_pool_t *pool, const char *host, hdns_rr_type_t type);

hdns_status_t hdns_resv_req_valid(const hdns_resv_req_t *req);

hdns_resv_req_t *hdns_resv_req_new(hdns_pool_t *pool, hdns_config_t *config);

void hdns_resv_req_free(hdns_resv_req_t *req);

hdns_resv_req_t *hdns_resv_req_clone(hdns_pool_t *pool, const hdns_resv_req_t *origin_req);

hdns_http_response_t *hdns_resv_send_req(hdns_pool_t *req_pool, hdns_resv_req_t *resv_req);

hdns_resv_resp_t *hdns_resv_resp_clone(hdns_pool_t *pool, const hdns_resv_resp_t *origin_resp);

void hdns_resv_resp_destroy(hdns_resv_resp_t *resp);

void hdns_parse_resv_resp(hdns_resv_req_t *resv_req,
                          hdns_http_response_t *http_resp,
                          hdns_pool_t *pool,
                          hdns_list_head_t *resv_resps);

char *hdns_resv_resp_to_str(hdns_pool_t *pool, hdns_resv_resp_t *resp);


HDNS_CPP_END


#endif
