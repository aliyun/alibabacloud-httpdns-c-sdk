//
// Created by cagaoshuai on 2024/3/26.
//

#include <apr_thread_mutex.h>

#include "hdns_define.h"
#include "hdns_log.h"

#include "hdns_session.h"

static apr_thread_mutex_t *g_hdns_session_stack_mutex = NULL;
static CURL *g_hdns_session_stack[HDNS_REQUEST_STACK_SIZE];
static int32_t g_hdns_session_stack_size = 0;
static hdns_pool_t *g_hdns_session_pool = NULL;
static volatile int32_t g_hdns_session_initialized = 0;


int hdns_session_pool_init(hdns_pool_t *parent_pool, int flags) {

    if (g_hdns_session_initialized++) {
        return HDNS_OK;
    }

    CURLcode ecode;
    int s;
    char buf[256];
    if ((ecode = curl_global_init(CURL_GLOBAL_ALL &
                                  ~((flags & HDNS_INIT_WINSOCK) ? 0 : CURL_GLOBAL_WIN32))) != CURLE_OK) {
        hdns_log_error("curl_global_init failure, code:%d %s.\n", ecode, curl_easy_strerror(ecode));
        return HDNS_ERROR;
    }
    if ((s = hdns_pool_create(&g_hdns_session_pool, parent_pool)) != APR_SUCCESS) {
        hdns_log_error("hdns_pool_create failure, code:%d %s.\n", s, apr_strerror(s, buf, sizeof(buf)));
        return HDNS_ERROR;
    }
    if ((s = apr_thread_mutex_create(&g_hdns_session_stack_mutex, APR_THREAD_MUTEX_DEFAULT, g_hdns_session_pool)) !=
        APR_SUCCESS) {
        hdns_log_error("apr_thread_mutex_create failure, code:%d %s.\n", s, apr_strerror(s, buf, sizeof(buf)));
        return HDNS_ERROR;
    }
    g_hdns_session_stack_size = 0;

    return HDNS_OK;
}

CURL *hdns_session_require() {
    CURL *curl_handle = NULL;

    apr_thread_mutex_lock(g_hdns_session_stack_mutex);
    if (g_hdns_session_stack_size > 0) {
        curl_handle = g_hdns_session_stack[--g_hdns_session_stack_size];
    }
    apr_thread_mutex_unlock(g_hdns_session_stack_mutex);

    // If we got one, deinitialize it for re-use
    if (curl_handle) {
        curl_easy_reset(curl_handle);
    } else {
        curl_handle = curl_easy_init();
    }
    return curl_handle;
}

void hdns_session_release(CURL *session) {
    apr_thread_mutex_lock(g_hdns_session_stack_mutex);

    // If the session stack is full, destroy this one
    // else put this one at the front of the session stack; we do this because
    // we want the most-recently-used curl handle to be re-used on the next
    // session, to maximize our chances of re-using a TCP connection before it
    // times out
    if (g_hdns_session_stack_size == HDNS_REQUEST_STACK_SIZE) {
        apr_thread_mutex_unlock(g_hdns_session_stack_mutex);
        curl_easy_cleanup(session);
    } else {
        g_hdns_session_stack[g_hdns_session_stack_size++] = session;
        apr_thread_mutex_unlock(g_hdns_session_stack_mutex);
    }
}

void hdns_session_pool_cleanup() {
    if (!g_hdns_session_initialized) {
        return;
    }
    if (--g_hdns_session_initialized) {
        return;
    }
    while (g_hdns_session_stack_size--) {
        curl_easy_cleanup(g_hdns_session_stack[g_hdns_session_stack_size]);
    }
    curl_global_cleanup();
    if (g_hdns_session_stack_mutex != NULL) {
        apr_thread_mutex_destroy(g_hdns_session_stack_mutex);
        g_hdns_session_stack_mutex = NULL;
    }
    if (g_hdns_session_pool != NULL) {
        hdns_pool_destroy(g_hdns_session_pool);
        g_hdns_session_pool = NULL;
    }
}

