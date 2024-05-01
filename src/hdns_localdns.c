//
// Created by cagaoshuai on 2024/4/19.
//

#include <apr_network_io.h>

#include "hdns_localdns.h"
#include "hdns_log.h"


hdns_resv_resp_t *hdns_localdns_resolve(hdns_pool_t *pool, const char *host, hdns_rr_type_t type) {
    apr_sockaddr_t *sa;
    apr_status_t rv;
    char *ip;
    hdns_pool_new(tmp_pool);
    hdns_resv_resp_t *resp = hdns_resv_resp_create_empty(pool, host, type);
    resp->from_localdns = true;
    apr_int32_t family = type == HDNS_RR_TYPE_A ? APR_INET : APR_INET6;
    rv = apr_sockaddr_info_get(&sa, host, family, 0, 0, tmp_pool);
    if (rv != APR_SUCCESS) {
        apr_pool_destroy(tmp_pool);
        return resp;
    }
    for (; sa; sa = sa->next) {
        apr_sockaddr_ip_get(&ip, sa);
        hdns_list_add(resp->ips, ip, hdns_to_list_clone_fn_t(apr_pstrdup));
    }
    hdns_pool_destroy(tmp_pool);
    return resp;
}