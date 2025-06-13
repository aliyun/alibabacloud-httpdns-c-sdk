//
// Created by caogaoshuai on 2024/1/22.
//

#include "hdns_http.h"
#include "hdns_ip.h"
#include "hdns_buf.h"
#include "hdns_localdns.h"
#include "hdns_log.h"

#include "hdns_client.h"

#define HDNS_MULTI_RESOLVE_SIZE 5

hdns_status_t hdns_fetch_resv_results(hdns_client_t *client, hdns_resv_req_t *resv_req, hdns_cache_t *cache);

hdns_status_t hdns_batch_fetch_resv_results(hdns_client_t *client,
                                            const hdns_list_head_t *hosts,
                                            hdns_query_type_t query_type,
                                            const char *client_ip,
                                            hdns_cache_t *cache);

static hdns_query_type_t unwrap_auto_query_type(hdns_net_detector_t *detector);

static int collect_resv_resp_in_cache_or_localdns(hdns_cache_t *cache,
                                                  const char *host,
                                                  const char *cache_key,
                                                  bool enable_expired_ip,
                                                  bool enable_failover_localdns,
                                                  hdns_list_head_t *results,
                                                  hdns_query_type_t query_type,
                                                  hdns_rr_type_t rr_type);

hdns_status_t hdns_do_single_resolve_with_req(hdns_client_t *client,
                                              hdns_resv_req_t *resv_req,
                                              hdns_list_head_t *results);

static APR_INLINE bool is_query_match(hdns_query_type_t query_type, hdns_rr_type_t rr_type) {
    switch (query_type) {
        case HDNS_QUERY_BOTH: {
            return true;
        }
        case HDNS_QUERY_IPV4: {
            return rr_type == HDNS_RR_TYPE_A;
        }
        case HDNS_QUERY_IPV6: {
            return rr_type == HDNS_RR_TYPE_AAAA;
        }
        default: {
            return false;
        }
    }
}

static int collect_resv_resp_in_cache_or_localdns(hdns_cache_t *cache,
                                                  const char *host,
                                                  const char *cache_key,
                                                  bool enable_expired_ip,
                                                  bool enable_failover_localdns,
                                                  hdns_list_head_t *results,
                                                  hdns_query_type_t query_type,
                                                  hdns_rr_type_t rr_type) {
    if (!is_query_match(query_type, rr_type)) {
        return HDNS_OK;
    }
    /*
     *  注意：结果统一在results->pool上
     */
    int ret = HDNS_ERROR;
    hdns_resv_resp_t *cache_resp = hdns_cache_table_get(cache, cache_key, rr_type);
    if (cache_resp != NULL && (!hdns_cache_entry_is_expired(cache_resp) || enable_expired_ip)) {
        hdns_list_add(results, cache_resp, hdns_to_list_clone_fn_t(hdns_resv_resp_clone));
        ret = HDNS_OK;
        goto cleanup;
    }
    if (enable_failover_localdns) {
        hdns_resv_resp_t *localdns_resp = hdns_localdns_resolve(results->pool, host, rr_type);
        hdns_list_add(results, localdns_resp, NULL);
        ret = HDNS_OK;
        goto cleanup;
    }
    hdns_resv_resp_t *empty_resp = hdns_resv_resp_create_empty(results->pool, host, rr_type);
    hdns_list_add(results, empty_resp, NULL);
    cleanup:
    hdns_resv_resp_destroy(cache_resp);
    return ret;
}

hdns_status_t hdns_do_single_resolve(hdns_client_t *client,
                                     const char *host,
                                     const hdns_query_type_t query_type,
                                     const bool using_cache,
                                     const char *client_ip,
                                     hdns_list_head_t *results) {
    hdns_pool_new(req_pool);
    hdns_resv_req_t *resv_req = hdns_resv_req_new(NULL, client->config);
    resv_req->query_type = query_type;
    resv_req->host = apr_pstrdup(req_pool, host);
    resv_req->query_type = query_type;
    if (hdns_str_is_not_blank(client_ip)) {
        resv_req->client_ip = apr_pstrdup(req_pool, client_ip);
    }
    resv_req->using_cache = using_cache;
    hdns_status_t status = hdns_do_single_resolve_with_req(client, resv_req, results);
    hdns_resv_req_free(resv_req);
    hdns_pool_destroy(req_pool);
    return status;
}

hdns_status_t hdns_do_single_resolve_with_req(hdns_client_t *client,
                                              hdns_resv_req_t *resv_req,
                                              hdns_list_head_t *results) {
    hdns_status_t status = hdns_resv_req_valid(resv_req);
    if (!hdns_status_is_ok(&status)) {
        return status;
    }
    hdns_cache_t *cache = NULL;
    hdns_pool_new(req_pool);
    if (resv_req->query_type == HDNS_QUERY_AUTO) {
        resv_req->query_type = unwrap_auto_query_type(client->net_detector);
    }
    char *cache_key = hdns_str_is_not_blank(resv_req->cache_key) ? resv_req->cache_key : resv_req->host;
    apr_thread_mutex_lock(client->config->lock);
    bool enable_failover_localdns = client->config->enable_failover_localdns;
    bool enable_expired_ip = client->config->enable_expired_ip;
    apr_thread_mutex_unlock(client->config->lock);
    if (resv_req->using_cache) {
        cache = client->cache;
        int32_t query_type_for_server = -1;
        switch (resv_req->query_type) {
            case HDNS_QUERY_BOTH: {
                hdns_resv_resp_t *ipv4_resp = hdns_cache_table_get(client->cache, cache_key, HDNS_RR_TYPE_A);
                hdns_resv_resp_t *ipv6_resp = hdns_cache_table_get(client->cache, cache_key, HDNS_RR_TYPE_AAAA);
                bool v4Invalid = ipv4_resp == NULL || hdns_cache_entry_is_expired(ipv4_resp);
                bool v6Invalid = ipv6_resp == NULL || hdns_cache_entry_is_expired(ipv6_resp);
                if (v4Invalid && v6Invalid) {
                    query_type_for_server = HDNS_QUERY_BOTH;
                } else if (v6Invalid) {
                    query_type_for_server = HDNS_QUERY_IPV6;
                } else if (v4Invalid) {
                    query_type_for_server = HDNS_QUERY_IPV4;
                }
                hdns_resv_resp_destroy(ipv4_resp);
                hdns_resv_resp_destroy(ipv6_resp);
                break;
            }
            case HDNS_QUERY_IPV4: {
                hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, cache_key, HDNS_RR_TYPE_A);
                bool invalid = resp == NULL || hdns_cache_entry_is_expired(resp);
                if (invalid) {
                    query_type_for_server = HDNS_QUERY_IPV4;
                }
                hdns_resv_resp_destroy(resp);
                break;
            }
            case HDNS_QUERY_IPV6: {
                hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, cache_key, HDNS_RR_TYPE_AAAA);
                bool invalid = resp == NULL || hdns_cache_entry_is_expired(resp);
                if (invalid) {
                    query_type_for_server = HDNS_QUERY_IPV6;
                }
                hdns_resv_resp_destroy(resp);
                break;
            }
            default:
                break;
        }
        if (query_type_for_server > 0) {
            resv_req->query_type = query_type_for_server;
            status = hdns_fetch_resv_results(client, resv_req, cache);
        }
    } else {
        cache = hdns_cache_table_create();
        status = hdns_fetch_resv_results(client, resv_req, cache);
    }

    if (!hdns_status_is_ok(&status) && !enable_failover_localdns && !enable_expired_ip) {
        goto cleanup;
    }
    int ipv4_collect_status = collect_resv_resp_in_cache_or_localdns(cache,
                                                                     resv_req->host,
                                                                     cache_key,
                                                                     enable_expired_ip,
                                                                     enable_failover_localdns,
                                                                     results,
                                                                     resv_req->query_type,
                                                                     HDNS_RR_TYPE_A);
    int ipv6_collect_status = collect_resv_resp_in_cache_or_localdns(cache,
                                                                     resv_req->host,
                                                                     cache_key,
                                                                     enable_expired_ip,
                                                                     enable_failover_localdns,
                                                                     results,
                                                                     resv_req->query_type,
                                                                     HDNS_RR_TYPE_AAAA);

    if (ipv4_collect_status == HDNS_OK && ipv6_collect_status == HDNS_OK) {
        status = hdns_status_ok(client->config->session_id);
    }
    cleanup:
    if (!resv_req->using_cache) {
        hdns_cache_table_cleanup(cache);
    }
    hdns_pool_destroy(req_pool);
    return status;
}

static hdns_status_t hdns_update_cache_on_net_change_with_type(hdns_net_chg_cb_task_t *task, hdns_rr_type_t rr_type) {
    hdns_client_t *client = task->param;
    hdns_status_t status;
    apr_time_t start = apr_time_now();
    hdns_query_type_t query_type = (rr_type == HDNS_RR_TYPE_A) ? HDNS_QUERY_IPV4 : HDNS_QUERY_IPV6;
    hdns_list_head_t *hosts = hdns_cache_get_keys(client->cache, rr_type);
    do {
        status = hdns_batch_fetch_resv_results(client, hosts, query_type, NULL, client->cache);
        hdns_log_debug("status:%d %s %s", status.code, status.error_code, status.error_msg);
    } while (!hdns_status_is_ok(&status)
             // 网络切换之后，存在一段时间网络不可用，需要等待重试
             && apr_time_sec(apr_time_now() - start) < 30
             && !task->stop_signal);
    hdns_list_free(hosts);
    return status;
}


void hdns_update_cache_on_net_change(hdns_net_chg_cb_task_t *task) {
    hdns_status_t status = hdns_update_cache_on_net_change_with_type(task, HDNS_RR_TYPE_A);
    if (hdns_status_is_ok(&status)) {
        hdns_update_cache_on_net_change_with_type(task, HDNS_RR_TYPE_AAAA);
    }
}


hdns_status_t hdns_do_batch_resolve(hdns_client_t *client,
                                    const hdns_list_head_t *hosts,
                                    hdns_query_type_t query_type,
                                    bool using_cache,
                                    const char *client_ip,
                                    hdns_list_head_t *results) {

    if (query_type == HDNS_QUERY_AUTO) {
        query_type = unwrap_auto_query_type(client->net_detector);
    }
    hdns_cache_t *cache = NULL;
    hdns_status_t status;
    hdns_pool_new(session_pool);
    apr_thread_mutex_lock(client->config->lock);
    bool enable_failover_localdns = client->config->enable_failover_localdns;
    bool enable_expired_ip = client->config->enable_expired_ip;
    apr_thread_mutex_unlock(client->config->lock);

    if (using_cache) {
        cache = client->cache;
        hdns_list_head_t *ipv4_hosts = hdns_list_new(session_pool);
        hdns_list_head_t *ipv6_hosts = hdns_list_new(session_pool);
        hdns_list_head_t *ipv46_hosts = hdns_list_new(session_pool);

        hdns_list_for_each_entry(host_cursor, hosts) {
            switch (query_type) {
                case HDNS_QUERY_BOTH: {
                    hdns_resv_resp_t *ipv4_resp = hdns_cache_table_get(client->cache,
                                                                       host_cursor->data,
                                                                       HDNS_RR_TYPE_A);
                    hdns_resv_resp_t *ipv6_resp = hdns_cache_table_get(client->cache,
                                                                       host_cursor->data,
                                                                       HDNS_RR_TYPE_AAAA);
                    bool v4Invalid = ipv4_resp == NULL || hdns_cache_entry_is_expired(ipv4_resp);
                    bool v6Invalid = ipv6_resp == NULL || hdns_cache_entry_is_expired(ipv6_resp);
                    if (v4Invalid && v6Invalid) {
                        hdns_list_add(ipv46_hosts, host_cursor->data, hdns_to_list_clone_fn_t(apr_pstrdup));
                    } else if (v4Invalid) {
                        hdns_list_add(ipv4_hosts, host_cursor->data, hdns_to_list_clone_fn_t(apr_pstrdup));
                    } else if (v6Invalid) {
                        hdns_list_add(ipv6_hosts, host_cursor->data, hdns_to_list_clone_fn_t(apr_pstrdup));
                    }
                    hdns_resv_resp_destroy(ipv4_resp);
                    hdns_resv_resp_destroy(ipv6_resp);
                    break;
                }
                case HDNS_QUERY_IPV4: {
                    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, host_cursor->data, HDNS_RR_TYPE_A);
                    bool invalid = resp == NULL || hdns_cache_entry_is_expired(resp);
                    if (invalid) {
                        hdns_list_add(ipv4_hosts, host_cursor->data, hdns_to_list_clone_fn_t(apr_pstrdup));
                    }
                    hdns_resv_resp_destroy(resp);
                    break;
                }
                case HDNS_QUERY_IPV6: {
                    hdns_resv_resp_t *resp = hdns_cache_table_get(client->cache, host_cursor->data, HDNS_RR_TYPE_AAAA);
                    bool invalid = resp == NULL || hdns_cache_entry_is_expired(resp);
                    if (invalid) {
                        hdns_list_add(ipv6_hosts, host_cursor->data, hdns_to_list_clone_fn_t(apr_pstrdup));
                    }
                    hdns_resv_resp_destroy(resp);
                    break;
                }
                default:
                    break;
            }
        }
        status = hdns_batch_fetch_resv_results(client,
                                               ipv4_hosts,
                                               HDNS_QUERY_IPV4,
                                               client_ip,
                                               cache);
        if (!hdns_status_is_ok(&status) && !enable_failover_localdns && !enable_expired_ip) {
            goto cleanup;
        }
        status = hdns_batch_fetch_resv_results(client,
                                               ipv6_hosts,
                                               HDNS_QUERY_IPV6,
                                               client_ip,
                                               cache);
        if (!hdns_status_is_ok(&status) && !enable_failover_localdns && !enable_expired_ip) {
            goto cleanup;
        }
        status = hdns_batch_fetch_resv_results(client,
                                               ipv46_hosts,
                                               HDNS_QUERY_BOTH,
                                               client_ip,
                                               cache);
        if (!hdns_status_is_ok(&status) && !enable_failover_localdns && !enable_expired_ip) {
            goto cleanup;
        }
    } else {
        cache = hdns_cache_table_create();
        status = hdns_batch_fetch_resv_results(client, hosts, query_type, client_ip, cache);
        if (!hdns_status_is_ok(&status) && !enable_failover_localdns && !enable_expired_ip) {
            goto cleanup;
        }
    }

    bool success = true;
    hdns_list_for_each_entry(host_cursor, hosts) {
        int ipv4_collect_status = collect_resv_resp_in_cache_or_localdns(cache,
                                                                         host_cursor->data,
                                                                         host_cursor->data,
                                                                         enable_expired_ip,
                                                                         enable_failover_localdns,
                                                                         results,
                                                                         query_type,
                                                                         HDNS_RR_TYPE_A);
        int ipv6_collect_status = collect_resv_resp_in_cache_or_localdns(cache,
                                                                         host_cursor->data,
                                                                         host_cursor->data,
                                                                         enable_expired_ip,
                                                                         enable_failover_localdns,
                                                                         results,
                                                                         query_type,
                                                                         HDNS_RR_TYPE_AAAA);
        success = success && (ipv4_collect_status == HDNS_OK);
        success = success && (ipv6_collect_status == HDNS_OK);
        if (!success) {
            break;
        }
    }
    if (success) {
        status = hdns_status_ok(client->config->session_id);
    }
    cleanup:
    if (!using_cache) {
        hdns_cache_table_cleanup(cache);
    }
    hdns_pool_destroy(session_pool);
    return status;
}


hdns_status_t hdns_batch_fetch_resv_results(hdns_client_t *client,
                                            const hdns_list_head_t *hosts,
                                            hdns_query_type_t query_type,
                                            const char *client_ip,
                                            hdns_cache_t *cache) {

    int host_count = 0;
    hdns_pool_new(session_pool);
    if (query_type == HDNS_QUERY_AUTO) {
        query_type = unwrap_auto_query_type(client->net_detector);
    }

    hdns_array_header_t *host_group = apr_array_make(session_pool, HDNS_MULTI_RESOLVE_SIZE, sizeof(char *));
    hdns_list_for_each_entry(host_cursor, hosts) {
        host_count++;
        *(char **) apr_array_push(host_group) = host_cursor->data;
        if (host_count % HDNS_MULTI_RESOLVE_SIZE != 0 && !hdns_list_is_end_node(host_cursor, hosts)) {
            continue;
        }
        hdns_pool_new_with_pp(req_pool, session_pool);

        hdns_resv_req_t *resv_req = hdns_resv_req_new(NULL, client->config);
        resv_req->using_multi = true;
        resv_req->host = apr_array_pstrcat(req_pool, host_group, ',');
        // 批量解析的cache_key只能是域名本身，不能单独设置
        resv_req->cache_key = NULL;
        resv_req->query_type = query_type;
        if (hdns_str_is_not_blank(client_ip)) {
            resv_req->client_ip = apr_pstrdup(req_pool, client_ip);
        }
        hdns_status_t status = hdns_fetch_resv_results(client, resv_req, cache);
        apr_array_clear(host_group);
        hdns_pool_destroy(req_pool);
        hdns_resv_req_free(resv_req);
        // 一批失败就失败，大概率服务异常
        if (!hdns_status_is_ok(&status)) {
            hdns_pool_destroy(session_pool);
            return status;
        }
    }
    hdns_pool_destroy(session_pool);
    return hdns_status_ok(client->config->session_id);
}


static hdns_query_type_t unwrap_auto_query_type(hdns_net_detector_t *detector) {
    hdns_net_type_t type = hdns_net_get_type(detector);
    switch (type) {
        case HDNS_IPV4_ONLY:
            return HDNS_QUERY_IPV4;
        case HDNS_IPV6_ONLY:
            return HDNS_QUERY_IPV6;
        default:
            return HDNS_QUERY_BOTH;
    }
}

typedef struct {
    hdns_pool_t *pool;
    hdns_cache_t *cache;
    hdns_resv_resp_t *resp;
} hdns_net_speed_cache_cb_fn_param_t;

void hdns_net_speed_cache_cb_fn(hdns_list_head_t *sorted_ips, void *user_params) {
    hdns_net_speed_cache_cb_fn_param_t *param = user_params;
    if (hdns_list_is_empty(sorted_ips)) {
        hdns_pool_destroy(param->pool);
        return;
    }
    hdns_list_for_each_entry_safe(sorted_cursor, sorted_ips) {
        hdns_list_for_each_entry_safe(cursor, param->resp->ips) {
            hdns_ip_t *sorted_ip = sorted_cursor->data;
            if (strcmp(sorted_ip->ip, cursor->data) == 0) {
                hdns_list_del(cursor);
                hdns_list_insert_tail(cursor, param->resp->ips);
            }
        }
    }
    hdns_cache_table_add(param->cache, param->resp);
    hdns_pool_destroy(param->pool);
}


static void hdns_apply_custom_ttl(hdns_client_t *client, hdns_resv_resp_t *resp) {
    apr_thread_mutex_lock(client->config->lock);
    int *ttl = apr_hash_get(client->config->custom_ttl_items,
                            resp->host,
                            APR_HASH_KEY_STRING);
    apr_thread_mutex_unlock(client->config->lock);
    if (ttl != NULL) {
        resp->ttl = (*ttl);
        resp->origin_ttl = (*ttl);
    }
}

static void hdns_probe_resv_resp_ips(hdns_client_t *client, hdns_resv_resp_t *resp) {
    apr_thread_mutex_lock(client->config->lock);
    int *port = apr_hash_get(client->config->ip_probe_items,
                             resp->cache_key,
                             APR_HASH_KEY_STRING);
    apr_thread_mutex_unlock(client->config->lock);
    if (port != NULL) {
        hdns_pool_new(pool);
        hdns_net_speed_cache_cb_fn_param_t *param = hdns_palloc(pool, sizeof(hdns_net_speed_cache_cb_fn_param_t));
        param->pool = pool;
        param->resp = hdns_resv_resp_clone(pool, resp);
        param->cache = client->cache;
        hdns_net_add_speed_detect_task(client->net_detector,
                                       hdns_net_speed_cache_cb_fn,
                                       param,
                                       resp->ips,
                                       (*port),
                                       client,
                                       &(client->state));
    }
}

hdns_status_t hdns_fetch_resv_results(hdns_client_t *client, hdns_resv_req_t *resv_req, hdns_cache_t *cache) {
    hdns_pool_new(req_pool);
    hdns_list_head_t *resv_resps = hdns_list_new(req_pool);
    int32_t retry_times = resv_req->retry_times;
    char resolver[256];
    hdns_status_t status;
    while (retry_times >= 0) {
        // 设置服务IP
        if (hdns_scheduler_get(client->scheduler, resolver) != HDNS_OK) {
            hdns_pool_destroy(req_pool);
            return hdns_status_error(HDNS_RESOLVE_FAIL,
                                     HDNS_RESOLVE_FAIL_CODE,
                                     "failed to get a resolver",
                                     client->config->session_id);
        }
        resv_req->resolver = apr_pstrdup(req_pool, resolver);
        // 触发请求
        hdns_http_response_t *http_resp = hdns_resv_send_req(req_pool, resv_req);
        // 建连失败
        if (http_resp->status <= 0) {
            status = hdns_status_error(HDNS_RESOLVE_FAIL,
                                       HDNS_RESOLVE_FAIL_CODE,
                                       http_resp->extra_info->reason,
                                       client->config->session_id);
            // 建连失败后全部进行重试
            hdns_scheduler_failover(client->scheduler, resolver);
            retry_times--;
            continue;
        } else if (http_resp->status == HDNS_HTTP_STATUS_OK) {
            // 服务端正常响应
            hdns_parse_resv_resp(resv_req, http_resp, req_pool, resv_resps);
            hdns_list_for_each_entry(entry_cursor, resv_resps) {
                hdns_apply_custom_ttl(client, entry_cursor->data);
                hdns_cache_table_add(cache, entry_cursor->data);
                hdns_probe_resv_resp_ips(client, entry_cursor->data);
            }
            status = hdns_status_ok(client->config->session_id);
            break;
        } else {
            // 服务端异常响应
            status = hdns_status_error(HDNS_RESOLVE_FAIL,
                                       HDNS_RESOLVE_FAIL_CODE,
                                       apr_pstrcat(req_pool, "Http response body: ",
                                                   hdns_buf_list_content(req_pool, http_resp->body), NULL),
                                       client->config->session_id);
            if (!hdns_http_should_retry(http_resp)) {
                break;
            }
            hdns_scheduler_failover(client->scheduler, resolver);
            retry_times--;
        }
    }
    hdns_pool_destroy(req_pool);
    return status;
}

