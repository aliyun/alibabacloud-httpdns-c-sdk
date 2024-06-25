//
// Created by caogaoshuai on 2024/6/25.
//
#include "hdns_utils.h"
#include "apr_env.h"
#include "hdns_log.h"

#define DEFAULT_WORK_DIR    "/tmp"

char *get_user_home_dir(apr_pool_t *p) {
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