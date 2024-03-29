//
// Created by caogaoshuai on 2024/1/11.
//

#include "httpdns_memory.h"
#include "httpdns_log.h"

#include "httpdns_net_stack_detector.h"


static int32_t test_udp_connect(struct sockaddr *sock_addr, sa_family_t sa_family, size_t addr_len) {
    int sock = socket(sa_family, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        return HTTPDNS_FAILURE;
    }
    int ret;
    do {
        ret = connect(sock, sock_addr, addr_len);
    } while (ret < 0 && errno == EINTR);
    int32_t result = (ret == 0) ? HTTPDNS_SUCCESS : HTTPDNS_FAILURE;
    do {
        ret = close(sock);
    } while (ret < 0 && errno == EINTR);
    return result;
}

static int32_t detect_ipv6_by_udp() {
    sa_family_t sa_family = AF_INET6;
    struct sockaddr_in6 server_addr;
    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = sa_family;
    server_addr.sin6_port = htons(HTTPDNS_PROBE_PORT);
    if (inet_pton(sa_family, HTTPDNS_IPV6_PROBE_ADDR, &server_addr.sin6_addr) <= 0) {
        httpdns_log_error("detect ipv6 by udp failed, inet_pton error, ipv6_probe_addr=%s", HTTPDNS_IPV6_PROBE_ADDR);
        return HTTPDNS_PARAMETER_ERROR;
    }
    return test_udp_connect((struct sockaddr *) &server_addr, sa_family, sizeof(server_addr));
}

static int32_t detect_ipv4_by_udp() {
    sa_family_t sa_family = AF_INET;
    struct sockaddr_in server_addr;
    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = sa_family;
    server_addr.sin_port = htons(HTTPDNS_PROBE_PORT);
    if (inet_pton(AF_INET, HTTPDNS_IPV4_PROBE_ADDR, &server_addr.sin_addr) <= 0) {
        httpdns_log_error("detect ipv4 by udp failed, inet_pton error, ipv4_probe_addr=%s", HTTPDNS_IPV4_PROBE_ADDR);
        return HTTPDNS_PARAMETER_ERROR;
    }
    return test_udp_connect((struct sockaddr *) &server_addr, sa_family, sizeof(server_addr));
}


httpdns_net_stack_type_t detect_net_stack_by_udp() {
    bool have_ipv4 = (detect_ipv4_by_udp() == HTTPDNS_SUCCESS);
    bool have_ipv6 = (detect_ipv6_by_udp() == HTTPDNS_SUCCESS);
    httpdns_net_stack_type_t net_stack_type = HTTPDNS_IP_STACK_UNKNOWN;
    if (have_ipv4) {
        httpdns_log_debug("detect ipv4 net stack by udp");
        httpdns_add_ipv4_net_type(net_stack_type);
    }
    if (have_ipv6) {
        httpdns_log_debug("detect ipv6 net stack by udp");
        httpdns_add_ipv6_net_type(net_stack_type);
    }
    return net_stack_type;
}


httpdns_net_stack_type_t detect_net_stack_by_dns(const char *probe_domain) {
    struct addrinfo hint, *answer, *curr;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    int status = getaddrinfo(probe_domain, NULL, &hint, &answer);
    if (status != 0) {
        httpdns_log_error("detect net stack by dns failed, getaddrinfo failed, errono=%d", status);
        return HTTPDNS_IP_STACK_UNKNOWN;
    }
    bool have_ipv4 = false, have_ipv6 = false;
    for (curr = answer; curr != NULL; curr = curr->ai_next) {
        if (curr->ai_family == AF_INET) {
            have_ipv4 = true;
        }
        if (curr->ai_family == AF_INET6) {
            have_ipv6 = true;
        }
    }
    freeaddrinfo(answer);
    httpdns_net_stack_type_t net_stack_type = HTTPDNS_IP_STACK_UNKNOWN;
    if (have_ipv4) {
        httpdns_log_debug("detect ipv4 net stack by dns");
        httpdns_add_ipv4_net_type(net_stack_type);
    }
    if (have_ipv6) {
        httpdns_log_debug("detect ipv6 net stack by dns");
        httpdns_add_ipv6_net_type(net_stack_type);
    }
    return net_stack_type;
}

httpdns_net_stack_type_t detect_net_stack(const char *probe_domain) {
    httpdns_net_stack_type_t net_stack_type = detect_net_stack_by_udp();
    if (net_stack_type != HTTPDNS_IP_STACK_UNKNOWN) {
        httpdns_log_info("detect net stack by udp, type=%d", net_stack_type);
        return net_stack_type;
    }
    if (NULL == probe_domain) {
        probe_domain = HTTPDNS_PROBE_DOMAIN;
    }
    net_stack_type = detect_net_stack_by_dns(probe_domain);
    if (net_stack_type != HTTPDNS_IP_STACK_UNKNOWN) {
        httpdns_log_debug("detect net stack by dns, type=%d", net_stack_type);
        return net_stack_type;
    }
    httpdns_log_info("no network stack available");
    return HTTPDNS_IP_STACK_UNKNOWN;
}

httpdns_net_stack_detector_t *httpdns_net_stack_detector_new() {
    httpdns_new_object_in_heap(detector, httpdns_net_stack_detector_t);
    detector->net_stack_type_cache = HTTPDNS_IP_STACK_UNKNOWN;
    detector->using_cache = true;
    detector->probe_domain = httpdns_sds_new(HTTPDNS_PROBE_DOMAIN);
    return detector;
}

void httpdns_net_stack_detector_free(httpdns_net_stack_detector_t *detector) {
    if (NULL == detector) {
        return;
    }
    if (NULL != detector->probe_domain) {
        httpdns_sds_free(detector->probe_domain);
    }
    free(detector);
}

void httpdns_net_stack_detector_update_cache(httpdns_net_stack_detector_t *detector) {
    if (NULL == detector) {
        httpdns_log_info("httpdns net stack detector update cache failed, detector is NULL");
        return;
    }
    httpdns_net_stack_type_t net_stack_type = detect_net_stack(detector->probe_domain);
    if (net_stack_type != HTTPDNS_IP_STACK_UNKNOWN) {
        httpdns_log_info("httpdns net stack detector update cache success, net_stack_type is %d", net_stack_type);
        detector->net_stack_type_cache = net_stack_type;
    }
}

void httpdns_net_stack_detector_set_using_cache(httpdns_net_stack_detector_t *detector, bool using_cache) {
    if (NULL == detector) {
        httpdns_log_info("httpdns net stack detector set using cache failed, detector is NULL");
        return;
    }
    detector->using_cache = using_cache;
}

void httpdns_net_stack_detector_set_probe_domain(httpdns_net_stack_detector_t *detector, const char *probe_domain) {
    httpdns_set_string_field(detector, probe_domain, probe_domain)
}


httpdns_net_stack_type_t httpdns_net_stack_type_get(httpdns_net_stack_detector_t *detector) {
    if (NULL == detector) {
        httpdns_log_info("httpdns get net stack failed, net stack detector is NULL");
        return HTTPDNS_IP_STACK_UNKNOWN;
    }
    httpdns_net_stack_type_t net_stack_type = detector->net_stack_type_cache;
    if (detector->using_cache && net_stack_type != HTTPDNS_IP_STACK_UNKNOWN) {
        httpdns_log_debug("httpdns get net stack success, hit cache, the value is %d", net_stack_type);
        return net_stack_type;
    }
    net_stack_type = detect_net_stack(detector->probe_domain);
    if (net_stack_type != HTTPDNS_IP_STACK_UNKNOWN) {
        detector->net_stack_type_cache = net_stack_type;
        return net_stack_type;
    }
    httpdns_log_info("no network stack available");
    return HTTPDNS_IP_STACK_UNKNOWN;
}





