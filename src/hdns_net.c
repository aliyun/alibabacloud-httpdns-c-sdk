//
// Created by caogaoshuai on 2024/1/11.
//
#include "hdns_log.h"
#include "hdns_string.h"
#include "hdns_net.h"
#include "hdns_ip.h"
#include "hdns_utils.h"


#if defined(__APPLE__) || defined(__linux__)

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <unistd.h>

static int32_t test_udp_connect(struct sockaddr *sock_addr, sa_family_t sa_family, size_t addr_len);

static int32_t detect_ipv6_by_udp();

static int32_t detect_ipv4_by_udp();

static hdns_net_type_t detect_net_stack_by_udp();

#elif _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")

static hdns_net_type_t detect_net_stack_by_winsock();

#endif

static hdns_net_type_t detect_net_stack_by_dns(const char *probe_domain);

static hdns_net_type_t detect_net_stack();

void hdns_net_type_detector_update_cache(hdns_net_chg_cb_task_t *task);


static void *APR_THREAD_FUNC hdns_net_chg_cb_task_runner(apr_thread_t *thread, void *data) {
    hdns_unused_var(thread);
    hdns_net_chg_cb_task_t *task = data;
    task->fn(task);
    apr_atomic_dec32(&task->parallelism);
    return NULL;
}

static void *APR_THREAD_FUNC hdns_net_change_detect_task(apr_thread_t *thread, void *data) {
    hdns_unused_var(thread);
    hdns_unused_var(data);
    hdns_net_detector_t *detector = data;
    while (!detector->change_detector->stop_signal) {
        if (hdns_net_is_changed(detector)) {
            hdns_log_debug("Networking change.");
            if (detector->change_detector->is_first) {
                detector->change_detector->is_first = false;
                continue;
            }
            apr_thread_mutex_lock(detector->change_detector->lock);
            hdns_list_for_each_entry_safe(cursor, detector->change_detector->cb_tasks) {
                hdns_net_chg_cb_task_t *task = cursor->data;
                if (apr_atomic_read32(&task->parallelism) > 0) {
                    continue;
                }
                apr_status_t s = apr_thread_pool_push(detector->thread_pool,
                                                      hdns_net_chg_cb_task_runner,
                                                      task, 0,
                                                      task->ownner);
                if (s != APR_SUCCESS) {
                    hdns_log_error("Submit task failed");
                }
                apr_atomic_inc32(&task->parallelism);
            }
            apr_thread_mutex_unlock(detector->change_detector->lock);
        }
        apr_sleep(APR_USEC_PER_SEC / 2);
    }
    hdns_log_info("Network monitoring task terminated.");
    return NULL;
}

static void *APR_THREAD_FUNC hdns_net_speed_detect_runner(apr_thread_t *thread, void *data) {
    hdns_unused_var(thread);
    hdns_unused_var(data);
    hdns_net_speed_detect_task_t *task = data;
    apr_status_t rv;
    apr_socket_t *sock = NULL;
    apr_sockaddr_t *sa = NULL;
    apr_time_t start;
    hdns_list_head_t *sorted_ips = hdns_list_new(task->pool);
    if (hdns_list_is_empty(task->ips)) {
       goto cleanup;
    }
    hdns_list_for_each_entry_safe(cursor, task->ips) {
        bool stop_signal = (*(task->ownner_state) == HDNS_STATE_STOPPING);
        if (stop_signal) {
            goto cleanup;
        }
        hdns_ip_t *ip = hdns_ip_create(task->pool, cursor->data);
        ip->rt = 30 * APR_USEC_PER_SEC;
        apr_int32_t family = APR_INET;
        if (hdns_is_valid_ipv4(cursor->data)) {
            family = APR_INET;
        } else if (hdns_is_valid_ipv6(cursor->data)) {
            family = APR_INET6;
        } else {
            continue;
        }
        rv = apr_sockaddr_info_get(&sa, cursor->data, family, task->port, 0, task->pool);
        if (rv != APR_SUCCESS) {
            apr_socket_close(sock);
            hdns_list_add(sorted_ips, ip, NULL);
            continue;
        }
        rv = apr_socket_create(&sock, sa->family, SOCK_STREAM, APR_PROTO_TCP, task->pool);
        if (rv != APR_SUCCESS) {
            apr_socket_close(sock);
            hdns_list_add(sorted_ips, ip, NULL);
            continue;
        }
        rv = apr_socket_timeout_set(sock, 3 * APR_USEC_PER_SEC);
        if (rv != APR_SUCCESS) {
            apr_socket_close(sock);
            hdns_list_add(sorted_ips, ip, NULL);
            continue;
        }
        start = apr_time_now();
        rv = apr_socket_connect(sock, sa);
        if (rv != APR_SUCCESS) {
            apr_socket_close(sock);
            hdns_list_add(sorted_ips, ip, NULL);
            continue;
        }
        apr_socket_close(sock);
        ip->rt = hdns_to_int(apr_time_now() - start);
        hdns_list_add(sorted_ips, ip, NULL);
    }
    hdns_list_sort(sorted_ips, hdns_to_list_cmp_fn_t(hdns_ip_cmp));
    cleanup:
    task->fn(sorted_ips, task->param);
    hdns_pool_destroy(task->pool);
    return NULL;
}

static void *APR_THREAD_FUNC hdns_net_speed_detect_task(apr_thread_t *thread, void *data) {
    hdns_unused_var(thread);
    hdns_unused_var(data);
    hdns_net_detector_t *detector = data;
    while (!detector->speed_detector->stop_signal) {
        apr_thread_mutex_lock(detector->speed_detector->lock);
        while (!detector->speed_detector->stop_signal && hdns_list_size(detector->speed_detector->tasks) <= 0) {
            // 避免hdns_net_detector_stop竞态条件，导致无法唤醒
            apr_thread_cond_timedwait(detector->speed_detector->not_empty_cond, detector->speed_detector->lock, apr_time_from_sec(5));
        }
        hdns_list_for_each_entry_safe(cursor, detector->speed_detector->tasks) {
            hdns_net_speed_detect_task_t *task = cursor->data;
            apr_status_t s = apr_thread_pool_push(detector->thread_pool,
                                                  hdns_net_speed_detect_runner,
                                                  task,
                                                  0,
                                                  task->owner);
            if (s != APR_SUCCESS) {
                hdns_log_error("Submit task failed");
            }
            hdns_list_del(cursor);
        }
        apr_thread_mutex_unlock(detector->speed_detector->lock);
        apr_sleep(APR_USEC_PER_SEC / 2);
    }
    hdns_log_info("Network speed monitoring task terminated.");
    return NULL;
}

#if defined(__APPLE__) || defined(__linux__)

static int32_t test_udp_connect(struct sockaddr *sock_addr, sa_family_t sa_family, size_t addr_len) {
    int sock = socket(sa_family, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        return HDNS_ERROR;
    }
    int ret;
    do {
        ret = connect(sock, sock_addr, addr_len);
    } while (ret < 0 && errno == EINTR);
    int32_t result = (ret == 0) ? HDNS_OK : HDNS_ERROR;
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
    server_addr.sin6_port = htons(HDNS_PROBE_PORT);
    if (inet_pton(sa_family, HDNS_IPV6_PROBE_ADDR, &server_addr.sin6_addr) <= 0) {
        hdns_log_error("detect ipv6 by udp failed, inet_pton error, ipv6_probe_addr=%s", HDNS_IPV6_PROBE_ADDR);
        return HDNS_ERROR;
    }
    return test_udp_connect((struct sockaddr *) &server_addr, sa_family, sizeof(server_addr));
}

static int32_t detect_ipv4_by_udp() {
    sa_family_t sa_family = AF_INET;
    struct sockaddr_in server_addr;
    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = sa_family;
    server_addr.sin_port = htons(HDNS_PROBE_PORT);
    if (inet_pton(AF_INET, HDNS_IPV4_PROBE_ADDR, &server_addr.sin_addr) <= 0) {
        hdns_log_error("detect ipv4 by udp failed, inet_pton error, ipv4_probe_addr=%s", HDNS_IPV4_PROBE_ADDR);
        return HDNS_ERROR;
    }
    return test_udp_connect((struct sockaddr *) &server_addr, sa_family, sizeof(server_addr));
}


static hdns_net_type_t detect_net_stack_by_udp() {
    bool have_ipv4 = (detect_ipv4_by_udp() == HDNS_OK);
    bool have_ipv6 = (detect_ipv6_by_udp() == HDNS_OK);
    hdns_net_type_t net_stack_type = HDNS_NET_UNKNOWN;
    if (have_ipv4) {
        hdns_log_debug("detect ipv4 net stack by udp");
        net_stack_type |= HDNS_IPV4_ONLY;
    }
    if (have_ipv6) {
        hdns_log_debug("detect ipv6 net stack by udp");
        net_stack_type |= HDNS_IPV6_ONLY;
    }
    return net_stack_type;
}

#elif defined(_WIN32)

static hdns_net_type_t detect_net_stack_by_winsock() {
    hdns_net_type_t net_type = HDNS_NET_UNKNOWN;
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            return net_type;
        }

        bool hasIPv4 = (LOBYTE(wsaData.wVersion) >= 2 && wsaData.szSystemStatus[0] != '\0');
        bool hasIPv6 = false;
        SOCKET sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
        if (sock != INVALID_SOCKET) {
            hasIPv6 = true;
            closesocket(sock);
        }
        if (hasIPv4) {
            net_type |= HDNS_IPV4_ONLY;
        }
        if (hasIPv6) {
            net_type |= HDNS_IPV6_ONLY;
        }
        WSACleanup();
        return net_type;
}
#endif

static hdns_net_type_t detect_net_stack_by_dns(const char *probe_domain) {
    struct addrinfo hint, *answer, *curr;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    int status = getaddrinfo(probe_domain, NULL, &hint, &answer);
    if (status != 0) {
        hdns_log_error("detect net stack by dns failed, getaddrinfo failed, errono=%d", status);
        return HDNS_NET_UNKNOWN;
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
    hdns_net_type_t net_stack_type = HDNS_NET_UNKNOWN;
    if (have_ipv4) {
        hdns_log_debug("detect ipv4 net stack by dns");
        net_stack_type |= HDNS_IPV4_ONLY;
    }
    if (have_ipv6) {
        hdns_log_debug("detect ipv6 net stack by dns");
        net_stack_type |= HDNS_IPV6_ONLY;
    }
    return net_stack_type;
}

static hdns_net_type_t detect_net_stack() {
    hdns_net_type_t net_stack_type;

#if defined(__APPLE__) || defined(__linux__)
    net_stack_type = detect_net_stack_by_udp();
#elif defined(_WIN32)
    net_stack_type = detect_net_stack_by_winsock();
#endif
    if (net_stack_type != HDNS_NET_UNKNOWN) {
        hdns_log_info("detect net stack by udp, type=%d", net_stack_type);
        return net_stack_type;
    }
    net_stack_type = detect_net_stack_by_dns(HDNS_PROBE_DOMAIN);
    if (net_stack_type != HDNS_NET_UNKNOWN) {
        hdns_log_debug("detect net stack by dns, type=%d", net_stack_type);
        return net_stack_type;
    }
    hdns_log_info("no network stack available");
    return HDNS_NET_UNKNOWN;
}

hdns_net_detector_t *hdns_net_detector_create(apr_thread_pool_t *thread_pool) {
    hdns_pool_new(pool);
    hdns_net_detector_t *detector = hdns_palloc(pool, sizeof(hdns_net_detector_t));
    detector->thread_pool = thread_pool;
    detector->pool = pool;

    detector->type_detector = hdns_palloc(detector->pool, sizeof(hdns_net_type_t));
    detector->type_detector->type = HDNS_NET_UNKNOWN;

    detector->change_detector = hdns_palloc(detector->pool, sizeof(hdns_net_change_detector_t));
    detector->change_detector->local_ips = NULL;
    detector->change_detector->is_first = true;
    detector->change_detector->stop_signal = false;
    detector->change_detector->cb_tasks = hdns_list_new(pool);
    apr_thread_mutex_create(&detector->change_detector->lock, APR_THREAD_MUTEX_DEFAULT, pool);
    hdns_net_add_chg_cb_task(detector,
                             HDNS_NET_CB_UPDATE_NET_TYPE,
                             (hdns_net_chg_cb_fn_t) hdns_net_type_detector_update_cache,
                             detector,
                             detector);
    apr_thread_pool_push(thread_pool,
                         hdns_net_change_detect_task,
                         detector,
                         0,
                         detector);


    detector->speed_detector = hdns_palloc(detector->pool, sizeof(hdns_net_speed_detector_t));
    detector->speed_detector->stop_signal = false;
    detector->speed_detector->tasks = hdns_list_new(pool);
    apr_thread_mutex_create(&detector->speed_detector->lock, APR_THREAD_MUTEX_DEFAULT, pool);
    apr_thread_cond_create(&detector->speed_detector->not_empty_cond, pool);

    apr_thread_pool_push(thread_pool,
                         hdns_net_speed_detect_task,
                         detector,
                         0,
                         detector);


    return detector;
}
void hdns_net_detector_stop(hdns_net_detector_t *detector) {
    detector->speed_detector->stop_signal = true;
    detector->change_detector->stop_signal = true;
    hdns_net_cancel_chg_cb_task(detector, detector);
    apr_thread_cond_broadcast(detector->speed_detector->not_empty_cond);
    apr_thread_pool_tasks_cancel(detector->thread_pool, detector);
}
void hdns_net_detector_cleanup(hdns_net_detector_t *detector) {
    apr_thread_mutex_destroy(detector->change_detector->lock);
    if (detector->change_detector->local_ips != NULL) {
        hdns_list_free(detector->change_detector->local_ips);
    }
    hdns_list_for_each_entry_safe(cursor, detector->change_detector->cb_tasks) {
        hdns_list_del(cursor);
        hdns_pool_destroy(cursor->pool);
    }
    apr_thread_mutex_destroy(detector->speed_detector->lock);
    apr_thread_cond_destroy(detector->speed_detector->not_empty_cond);
    hdns_pool_destroy(detector->pool);
}


void hdns_net_type_detector_update_cache(hdns_net_chg_cb_task_t *task) {
    hdns_net_detector_t *detector = task->param;
    hdns_net_type_t net_stack_type;
    apr_time_t start = apr_time_now();
    do {
        net_stack_type = detect_net_stack();
        apr_sleep(APR_USEC_PER_SEC / 2);
        //网络切换后，需要等待一段时间后才能探测到网络栈
    } while (net_stack_type == HDNS_NET_UNKNOWN
             && apr_time_sec(apr_time_now() - start) < 30
             && !task->stop_signal);
    detector->type_detector->type = net_stack_type;
}

hdns_net_type_t hdns_net_get_type(hdns_net_detector_t *detector) {
    if (NULL == detector) {
        hdns_log_info("httpdns get net stack failed, net stack detector is NULL");
        return HDNS_NET_UNKNOWN;
    }
    hdns_net_type_t net_stack_type = detector->type_detector->type;
    if (net_stack_type != HDNS_NET_UNKNOWN) {
        hdns_log_debug("httpdns get net stack success, hit cache, the value is %d", net_stack_type);
        return net_stack_type;
    }
    net_stack_type = detect_net_stack();
    if (net_stack_type != HDNS_NET_UNKNOWN) {
        detector->type_detector->type = net_stack_type;
        return net_stack_type;
    }
    hdns_log_info("no network stack available");
    return HDNS_NET_UNKNOWN;
}


bool hdns_net_is_changed(hdns_net_detector_t *detector) {
    if (NULL == detector) {
        return false;
    }
    hdns_pool_new(pool);
    hdns_list_head_t *new_local_ips = hdns_list_new(pool);
    hdns_net_change_detector_t *change_detector = detector->change_detector;

    apr_thread_mutex_lock(change_detector->lock);

    bool is_changed = false;

#if defined(__APPLE__) || defined(__linux__)
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char ip[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        hdns_pool_destroy(pool);
        hdns_log_error("getifaddrs failed:%s.", strerror(errno));
        return false;
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? sizeof(struct sockaddr_in) :
                            sizeof(struct sockaddr_in6),
                            ip, NI_MAXHOST,
                            NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                hdns_log_error("getnameinfo() failed: %s\n", gai_strerror(s));
                hdns_pool_destroy(pool);
                return false;
            }
            if (hdns_is_valid_ipv6(ip) || hdns_is_valid_ipv4(ip)) {
                hdns_list_add(new_local_ips, ip, hdns_to_list_clone_fn_t(apr_pstrdup));
                if (hdns_list_is_empty(change_detector->local_ips)) {
                    is_changed = true;
                } else if (NULL == hdns_list_search(change_detector->local_ips, ip,
                                                    hdns_to_list_search_fn_t(hdns_str_search))) {
                    is_changed = true;
                }
            }
        }
    }
    freeifaddrs(ifaddr);
#elif defined(_WIN32)
    WSADATA wsaData;
    struct addrinfo* result = NULL;
    struct addrinfo* ptr = NULL;
    struct addrinfo hints;
    char hostname[NI_MAXHOST];

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        hdns_log_error("WSAStartup failed.\n");
        hdns_pool_destroy(pool);
        return false;
    }

    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        hdns_log_error("gethostname failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        hdns_pool_destroy(pool);
        return false;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, NULL, &hints, &result) != 0) {
        hdns_log_error("gethostname failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        hdns_pool_destroy(pool);
        return false;
    }

    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        void* addr;
        char ip[INET6_ADDRSTRLEN];

        if (ptr->ai_family == AF_INET) {
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)ptr->ai_addr;
            addr = &(ipv4->sin_addr);
        }
        else {
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)ptr->ai_addr;
            addr = &(ipv6->sin6_addr);
        }

        inet_ntop(ptr->ai_family, addr, ip, sizeof(ip));

        if (hdns_is_valid_ipv6(ip) || hdns_is_valid_ipv4(ip)) {
            hdns_list_add(new_local_ips, ip, hdns_to_list_clone_fn_t(apr_pstrdup));
            if (hdns_list_is_empty(change_detector->local_ips)) {
                is_changed = true;
            }
            else if (NULL == hdns_list_search(change_detector->local_ips, ip,
                hdns_to_list_search_fn_t(hdns_str_search))) {
                is_changed = true;
            }
        }

    }
    freeaddrinfo(result);
    WSACleanup();
#endif
    if (hdns_list_size(new_local_ips) != hdns_list_size(change_detector->local_ips)) {
        is_changed = true;
    }
    if (is_changed) {
        if (change_detector->local_ips != NULL) {
            hdns_pool_destroy(change_detector->local_ips->pool);
        }
        detector->change_detector->local_ips = new_local_ips;
    } else {
        hdns_pool_destroy(pool);
    }
    apr_thread_mutex_unlock(detector->change_detector->lock);
    return is_changed;
}

void hdns_net_add_chg_cb_task(hdns_net_detector_t *detector,
                              hdns_net_chg_cb_type_t type,
                              hdns_net_chg_cb_fn_t fn,
                              void *param,
                              void *owner) {
    if (NULL == detector) {
        return;
    }
    apr_thread_mutex_lock(detector->change_detector->lock);
    hdns_list_for_each_entry_safe(cusor, detector->change_detector->cb_tasks) {
        hdns_net_chg_cb_task_t *callback = cusor->data;
        if (callback->ownner == owner && callback->type == type) {
            apr_thread_mutex_unlock(detector->change_detector->lock);
            return;
        }
    }
    hdns_pool_new(pool);
    hdns_list_node_t *entry = hdns_palloc(pool, sizeof(hdns_list_node_t));
    hdns_net_chg_cb_task_t *task = hdns_palloc(pool, sizeof(hdns_net_chg_cb_task_t));
    task->fn = fn;
    task->type = type;
    task->param = param;
    task->ownner = owner;
    task->stop_signal = false;
    task->parallelism = 0;
    entry->data = task;
    entry->pool = pool;
    hdns_list_insert_tail(entry, detector->change_detector->cb_tasks);
    apr_thread_mutex_unlock(detector->change_detector->lock);
}

void hdns_net_cancel_chg_cb_task(hdns_net_detector_t *detector, void *owner) {
    if (NULL == detector) {
        return;
    }
    apr_thread_mutex_lock(detector->change_detector->lock);
    hdns_list_for_each_entry_safe(cursor, detector->change_detector->cb_tasks) {
        hdns_net_chg_cb_task_t *callback = cursor->data;
        if (callback->ownner == owner) {
            callback->stop_signal = true;
            hdns_list_del(cursor);
            hdns_pool_destroy(cursor->pool);
        }
    }
    apr_thread_mutex_unlock(detector->change_detector->lock);
}


void hdns_net_add_speed_detect_task(hdns_net_detector_t *detector,
                                    hdns_net_speed_cb_fn_t fn,
                                    void *param,
                                    hdns_list_head_t *ips,
                                    int port,
                                    void *owner,
                                    hdns_state_e *ownner_state) {
    if (NULL == detector) {
        return;
    }
    hdns_pool_new(pool);
    hdns_net_speed_detect_task_t *task = hdns_palloc(pool, sizeof(hdns_net_speed_detect_task_t));
    task->pool = pool;
    task->ips = hdns_list_new(pool);
    hdns_list_dup(task->ips, ips, hdns_to_list_clone_fn_t(apr_pstrdup));
    task->port = port;
    task->fn = fn;
    task->param = param;
    task->owner = owner;
    task->ownner_state = ownner_state;
    hdns_list_node_t *entry = hdns_palloc(pool, sizeof(hdns_list_node_t));
    entry->data = task;
    entry->pool = pool;
    apr_thread_mutex_lock(detector->speed_detector->lock);
    hdns_list_insert_tail(entry, detector->speed_detector->tasks);
    apr_thread_cond_signal(detector->speed_detector->not_empty_cond);
    apr_thread_mutex_unlock(detector->speed_detector->lock);
}