//
// Created by caogaoshuai on 2024/2/5.
//
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <string.h>

#include "httpdns_ip.h"
#include "httpdns_log.h"
#include "httpdns_time.h"

#include "httpdns_localdns.h"

static httpdns_resolve_result_t *create_localdns_result(const char *host) {
    httpdns_resolve_result_t *result = httpdns_resolve_result_new();
    result->host = httpdns_sds_new(host);
    result->query_ts = httpdns_time_now();
    result->origin_ttl = 0;
    result->ttl = 0;
    result->cache_key = httpdns_sds_new(host);
    result->hit_cache = false;
    return result;
}

httpdns_resolve_result_t *httpdns_localdns_resolve_host(const char *host) {
    struct addrinfo hint, *answer, *curr;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    int status = getaddrinfo(host, NULL, &hint, &answer);
    if (status != 0) {
        httpdns_log_error("resolve host by localhost failed, getaddrinfo failed, errono=%d", status);
        return NULL;
    }
    void *addr;
    char ipstr[INET6_ADDRSTRLEN];
    httpdns_resolve_result_t *result = create_localdns_result(host);
    for (curr = answer; curr != NULL; curr = curr->ai_next) {
        if (curr->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *) curr->ai_addr;
            addr = &(ipv4->sin_addr);
            inet_ntop(curr->ai_family, addr, ipstr, sizeof ipstr);
            httpdns_list_add(&result->ips, ipstr, to_httpdns_data_clone_func(httpdns_ip_new));
        }
        if (curr->ai_family == AF_INET6) {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) curr->ai_addr;
            addr = &(ipv6->sin6_addr);
            inet_ntop(curr->ai_family, addr, ipstr, sizeof ipstr);
            httpdns_list_add(&result->ipsv6, ipstr, to_httpdns_data_clone_func(httpdns_ip_new));
        }
    }
    freeaddrinfo(answer);
    return result;
}