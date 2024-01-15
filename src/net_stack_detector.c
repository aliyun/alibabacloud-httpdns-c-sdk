//
// Created by cagaoshuai on 2024/1/11.
//

#include "net_stack_detector.h"


static int32_t test_udp_connect(struct sockaddr *sock_addr, sa_family_t sa_family, size_t addr_len) {
    int sock = socket(sa_family, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
        return HTTPDNS_FAILURE;
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

static int32_t have_ipv6_by_udp() {
    sa_family_t sa_family = AF_INET6;
    struct sockaddr_in6 server_addr;
    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = sa_family;
    server_addr.sin6_port = htons(PROBE_PORT);
    if (inet_pton(sa_family, IPV6_PROBE_ADDR, &server_addr.sin6_addr) <= 0) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    return test_udp_connect((struct sockaddr *) &server_addr, sa_family, sizeof(server_addr));
}

static int32_t have_ipv4_by_udp() {
    sa_family_t sa_family = AF_INET;
    struct sockaddr_in server_addr;
    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = sa_family;
    server_addr.sin_port = htons(PROBE_PORT);
    if (inet_pton(AF_INET, IPV4_PROBE_ADDR, &server_addr.sin_addr) <= 0) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    return test_udp_connect((struct sockaddr *) &server_addr, sa_family, sizeof(server_addr));
}


net_stack_type_t test_net_stack_by_udp() {
    bool have_ipv4 = (have_ipv4_by_udp() == HTTPDNS_SUCCESS);
    bool have_ipv6 = (have_ipv6_by_udp() == HTTPDNS_SUCCESS);
    net_stack_type_t net_stack_type = IP_STACK_UNKNOWN;
    if(have_ipv4) {
        ADD_IPV4_NET_TYPE(net_stack_type);
    }
    if(have_ipv6){
        ADD_IPV6_NET_TYPE(net_stack_type);
    }
    return net_stack_type;
}


net_stack_type_t test_net_stack_by_dns(const char *probe_domain) {
    struct addrinfo hint, *answer, *curr;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    int status = getaddrinfo(probe_domain, NULL, &hint, &answer);
    if (status != 0) {
        return IP_STACK_UNKNOWN;
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
    net_stack_type_t net_stack_type = IP_STACK_UNKNOWN;
    if(have_ipv4) {
        ADD_IPV4_NET_TYPE(net_stack_type);
    }
    if(have_ipv6) {
        ADD_IPV6_NET_TYPE(net_stack_type);
    }
    return net_stack_type;
}

net_stack_type_t test_net_stack(const char *probe_domain) {
    net_stack_type_t net_stack_type = test_net_stack_by_udp();
    if (net_stack_type != IP_STACK_UNKNOWN) {
        return net_stack_type;
    }
    if (NULL == probe_domain) {
        probe_domain = PROBE_DOMAIN;
    }
    net_stack_type = test_net_stack_by_dns(probe_domain);
    if (net_stack_type != IP_STACK_UNKNOWN) {
        return net_stack_type;
    }
    return IP_STACK_UNKNOWN;
}

net_stack_detector_t *create_net_stack_detector() {
    net_stack_detector_t *detector_ptr = (net_stack_detector_t *) malloc(sizeof(net_stack_detector_t));
    memset(detector_ptr, 0, sizeof(net_stack_detector_t));
    detector_ptr->net_stack_type_cache = IP_STACK_UNKNOWN;
    detector_ptr->using_cache = true;
    detector_ptr->probe_domain = sdsnew(PROBE_DOMAIN);
    return detector_ptr;
}

void destroy_net_stack_detector(net_stack_detector_t *detector) {
    if (NULL != detector) {
        if (NULL != detector->probe_domain) {
            sdsfree(detector->probe_domain);
        }
        free(detector);
    }
}

void net_stack_detector_update_cache(net_stack_detector_t *detector) {
    if (NULL == detector) {
        return;
    }
    net_stack_type_t net_stack_type = test_net_stack(detector->probe_domain);
    if (net_stack_type != IP_STACK_UNKNOWN) {
        detector->net_stack_type_cache = net_stack_type;
    }
}

void net_stack_detector_set_using_cache(net_stack_detector_t *detector, bool using_cache) {
    if (NULL == detector) {
        return;
    }
    detector->using_cache = using_cache;
}

void net_stack_detector_set_probe_domain(net_stack_detector_t *detector, const char *probe_domain) {
    if (NULL != detector && NULL != probe_domain) {
        detector->probe_domain = sdsnew(probe_domain);
    }
}


net_stack_type_t get_net_stack_type(net_stack_detector_t *detector) {
    if (NULL == detector) {
        return IP_STACK_UNKNOWN;
    }
    net_stack_type_t net_stack_type = detector->net_stack_type_cache;
    if (detector->using_cache && net_stack_type != IP_STACK_UNKNOWN) {
        return net_stack_type;
    }
    net_stack_type = test_net_stack(detector->probe_domain);
    if (net_stack_type != IP_STACK_UNKNOWN) {
        detector->net_stack_type_cache = net_stack_type;
        return net_stack_type;
    }
    return IP_STACK_UNKNOWN;
}





