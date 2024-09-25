//
// Created by caogaoshuai on 2024/6/25.
//
#include "hdns_utils.h"
#include "apr_env.h"
#include "hdns_log.h"
#include "apr_md5.h"


#if defined(__APPLE__) || defined(__linux__)

#include <arpa/inet.h>
#include <netinet/in.h>

#elif _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#define DEFAULT_WORK_DIR    "/tmp"

char *hdns_get_user_home_dir(hdns_pool_t *p) {
    char *home_dir = NULL;
    apr_status_t rv = APR_SUCCESS;

#if defined(__APPLE__) || defined(__linux__)
    rv = apr_env_get(&home_dir, "HOME", p);
#elif defined(_WIN32)
    rv = apr_env_get(&home_dir, "USERPROFILE", p);
#endif
    if (rv != APR_SUCCESS || home_dir == NULL) {
        char err_msg[128];
        apr_strerror(rv, err_msg, sizeof(err_msg));
        hdns_log_info("Failed to get user home directory: %s", err_msg); // Default fallback log
        return DEFAULT_WORK_DIR;
    }
    return home_dir;
}

bool hdns_is_valid_ipv6(const char *ipv6) {
    struct in6_addr addr6;
    return inet_pton(AF_INET6, ipv6, &addr6) == 1;
}

bool hdns_is_valid_ipv4(const char *ip) {
    struct in_addr addr;
    return inet_pton(AF_INET, ip, &addr) == 1;
}

void hdns_md5(const char *content, size_t size, char *digest) {
    apr_md5_ctx_t ctx;
    apr_md5_init(&ctx);
    apr_md5_update(&ctx, content, size);
    unsigned char binary_digest[HDNS_MD5_STRING_LEN / 2];
    apr_md5_final(binary_digest, &ctx);
    hdns_encode_hex(binary_digest, HDNS_MD5_STRING_LEN / 2, digest);
}


void hdns_encode_hex(const unsigned char *data, size_t size, char *hex) {
    const char hex_digits[] = "0123456789abcdef";
    for (int32_t i = 0; i < size; i++) {
        hex[i * 2] = hex_digits[(data[i] >> 4) & 0x0F];
        hex[i * 2 + 1] = hex_digits[data[i] & 0x0F];
    }
    hex[2 * size] = '\0';
}