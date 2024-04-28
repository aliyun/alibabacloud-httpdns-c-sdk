//
// Created by cagaoshuai on 2024/4/19.
//
#ifdef __unix__
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#endif
#include <string.h>

#include "hdns_localdns.h"
#include "hdns_log.h"


hdns_resv_resp_t *hdns_localdns_resolve(hdns_pool_t *pool, const char *host, hdns_rr_type_t type) {
    struct addrinfo hint, *answer, *curr;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    int status = getaddrinfo(host, NULL, &hint, &answer);
    hdns_resv_resp_t *resp = hdns_resv_resp_create_empty(pool, host, type);
    resp->from_localdns = true;
    if (status != 0) {
        hdns_log_error("resolve host by localhost failed, getaddrinfo failed, errono=%d, error_msg=%s",
                       status,
                       gai_strerror(status));
        return resp;
    }
    void *addr;
    char ipstr[INET6_ADDRSTRLEN];
    for (curr = answer; curr != NULL; curr = curr->ai_next) {
        if (curr->ai_family == AF_INET && type == HDNS_RR_TYPE_A) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *) curr->ai_addr;
            addr = &(ipv4->sin_addr);
            inet_ntop(curr->ai_family, addr, ipstr, sizeof ipstr);
            hdns_list_add(resp->ips, ipstr, hdns_to_list_clone_fn_t(apr_pstrdup));
        }
        if (curr->ai_family == AF_INET6 && type == HDNS_RR_TYPE_AAAA) {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) curr->ai_addr;
            addr = &(ipv6->sin6_addr);
            inet_ntop(curr->ai_family, addr, ipstr, sizeof ipstr);
            hdns_list_add(resp->ips, ipstr, hdns_to_list_clone_fn_t(apr_pstrdup));
        }
    }
    freeaddrinfo(answer);
    return resp;
}