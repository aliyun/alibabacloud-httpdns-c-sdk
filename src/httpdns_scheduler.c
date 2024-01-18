//
// Created by cagaoshuai on 2024/1/11.
//
#include "httpdns_scheduler.h"

static void print_resolve_servers(struct list_head *servers) {
    size_t server_size = httpdns_list_size(servers);
    printf("[");
    for (int i = 0; i < server_size; i++) {
        httpdns_resolve_server_t *server = httpdns_list_get(servers, i);
        printf(" { server=%s, response_time_ms=%d } ", server->server, server->response_time_ms);
    }
    printf("]\n");
}

void httpdns_scheduler_print_resolve_servers(httpdns_scheduler_t *scheduler) {
    if (NULL != scheduler) {
        printf("Resolve servers:\n");
        print_resolve_servers(&scheduler->ipv4_resolve_servers);
        print_resolve_servers(&scheduler->ipv6_resolve_servers);
    }
}


httpdns_scheduler_t *create_httpdns_scheduler(httpdns_config_t *config) {
    if (httpdns_config_is_valid(config) != HTTPDNS_SUCCESS) {
        return NULL;
    }
    httpdns_scheduler_t *scheduler = (httpdns_scheduler_t *) malloc(sizeof(httpdns_scheduler_t));
    memset(scheduler, 0, sizeof(httpdns_scheduler_t));
    httpdns_list_init(&scheduler->ipv4_resolve_servers);
    httpdns_list_init(&scheduler->ipv6_resolve_servers);
    scheduler->net_stack_detector = create_net_stack_detector();
    net_stack_detector_set_probe_domain(scheduler->net_stack_detector, config->probe_domain);
    scheduler->config = config;
    return scheduler;
}

void destroy_httpdns_scheduler(httpdns_scheduler_t *scheduler) {
    if (NULL == scheduler) {
        return;
    }
    if (NULL != scheduler->net_stack_detector) {
        destroy_net_stack_detector(scheduler->net_stack_detector);
    }
    httpdns_list_free(&scheduler->ipv4_resolve_servers, DATA_FREE_FUNC(destroy_httpdns_resolve_server));
    httpdns_list_free(&scheduler->ipv6_resolve_servers, DATA_FREE_FUNC(destroy_httpdns_resolve_server));
    free(scheduler);
}

static httpdns_resolve_server_t *clone_httpdns_resolve_server(const httpdns_resolve_server_t *origin_resolver) {
    if (NULL == origin_resolver) {
        return NULL;
    }
    httpdns_resolve_server_t *resolver = (httpdns_resolve_server_t *) malloc(sizeof(httpdns_resolve_server_t));
    if (NULL != origin_resolver->server) {
        resolver->server = sdsnew(origin_resolver->server);
    }
    resolver->response_time_ms = origin_resolver->response_time_ms;
    return resolver;
}

static void httpdns_parse_resolvers(cJSON *c_json_body, const char *item_name, struct list_head *dst_resolvers) {
    cJSON *resolvers = cJSON_GetObjectItem(c_json_body, item_name);
    if (NULL != resolvers) {
        size_t resolve_num = cJSON_GetArraySize(resolvers);
        struct list_head resolve_list;
        httpdns_list_init(&resolve_list);
        for (int i = 0; i < resolve_num; i++) {
            cJSON *resolver_json = cJSON_GetArrayItem(resolvers, i);
            httpdns_resolve_server_t *resolver = create_httpdns_resolve_server(resolver_json->valuestring);
            httpdns_list_add(&resolve_list, resolver, (data_clone_function_ptr_t) clone_httpdns_resolve_server);
            destroy_httpdns_resolve_server(resolver);
        }
        if (httpdns_list_size(&resolve_list) > 0) {
            httpdns_list_free(dst_resolvers, DATA_FREE_FUNC(destroy_httpdns_resolve_server));
            // 只有当不为空时，才更新
            httpdns_list_dup(dst_resolvers, &resolve_list, DATA_CLONE_FUNC(clone_httpdns_resolve_server));
            httpdns_list_free(&resolve_list, DATA_FREE_FUNC(destroy_httpdns_resolve_server));
            httpdns_list_shuffle(dst_resolvers);
        }
    }
}

static void httpdns_parse_body(void *response_body, httpdns_scheduler_t *scheduler) {
    cJSON *c_json_body = cJSON_Parse(response_body);
    if (NULL == c_json_body) {
        return;
    }
    httpdns_parse_resolvers(c_json_body, JSON_BODY_IPV4_RESOLVERS_ITEM, &scheduler->ipv4_resolve_servers);
    httpdns_parse_resolvers(c_json_body, JSON_BODY_IPV6_RESOLVERS_ITEM, &scheduler->ipv6_resolve_servers);
    cJSON_Delete(c_json_body);
}

int32_t httpdns_scheduler_refresh_resolve_servers(httpdns_scheduler_t *scheduler) {
    if (NULL == scheduler || NULL == scheduler->config) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    u_int32_t net_stack_type = get_net_stack_type(scheduler->net_stack_detector);
    httpdns_config_t *config = scheduler->config;
    struct list_head *boot_servers;

    if (IPV6_ONLY == net_stack_type) {
        boot_servers = &config->ipv6_boot_servers;
    } else {
        boot_servers = &config->ipv4_boot_servers;
    }
    size_t boot_server_num = httpdns_list_size(boot_servers);
    if (boot_server_num <= 0) {
        return HTTPDNS_FAILURE;
    }
    const char *http_scheme = config->using_https ? HTTPS_SCHEME : HTTP_SCHEME;
    for (int i = 0; i < boot_server_num; i++) {
        sds url = sdsnew(http_scheme);
        url = sdscat(url, httpdns_list_get(boot_servers, i));
        url = sdscat(url, "/");
        url = sdscat(url, config->account_id);
        url = sdscat(url, "/ss?platform=linux&sdk_version=");
        url = sdscat(url, config->sdk_version);
        httpdns_http_request_t *request = create_httpdns_http_request(url, config->timeout_ms, NULL);
        httpdns_http_response_t *response;
        httpdns_http_single_request_exchange(request, &response);
        if (response->http_status == HTTP_STATUS_OK) {
            httpdns_parse_body(response->body, scheduler);
        }
        sdsfree(url);
        destroy_httpdns_http_request(request);
        destroy_httpdns_http_response(response);
    }
    return HTTPDNS_SUCCESS;
}

static httpdns_resolve_server_t *search_resolve_server(struct list_head *resolve_servers, char *resolve_server_name) {
    if (NULL == resolve_servers) {
        return NULL;
    }
    size_t resolve_server_size = httpdns_list_size(resolve_servers);
    for (int i = 0; i < resolve_server_size; i++) {
        httpdns_resolve_server_t *target_server = httpdns_list_get(resolve_servers, i);
        if (strcmp(target_server->server, resolve_server_name) == 0) {
            return target_server;
        }
    }
    return NULL;
}

static int32_t update_server_response_time(int32_t old_rt_val, int32_t new_rt_val) {
    if (old_rt_val == DEFAULT_RESOLVE_SERVER_RT) {
        return new_rt_val;
    }
    if (new_rt_val * DELTA_WEIGHT_UPDATE_RATION > old_rt_val) {
        return old_rt_val;
    }
    return (int32_t) (new_rt_val * DELTA_WEIGHT_UPDATE_RATION + old_rt_val * (1.0 - DELTA_WEIGHT_UPDATE_RATION));
}

void httpdns_scheduler_update_server_rt(httpdns_scheduler_t *scheduler, char *resolve_server_name, int32_t new_time_cost_ms) {
    if (IS_BLANK_SDS(resolve_server_name) || NULL == scheduler || new_time_cost_ms <= 0) {
        return;
    }
    httpdns_resolve_server_t *resolve_server = search_resolve_server(&scheduler->ipv4_resolve_servers,
                                                                     resolve_server_name);
    if (NULL != resolve_server) {
        resolve_server->response_time_ms = update_server_response_time(resolve_server->response_time_ms,
                                                                       new_time_cost_ms);
    }
    resolve_server = search_resolve_server(&scheduler->ipv6_resolve_servers, resolve_server_name);
    if (NULL != resolve_server) {
        resolve_server->response_time_ms = update_server_response_time(resolve_server->response_time_ms,
                                                                       new_time_cost_ms);
    }
}

void httpdns_scheduler_get_resolve_server(httpdns_scheduler_t *scheduler, char **resolve_server_ptr) {
    if (NULL == scheduler) {
        return;
    }
    u_int32_t net_stack_type = get_net_stack_type(scheduler->net_stack_detector);
    struct list_head *resolve_servers;
    if (IPV6_ONLY == net_stack_type) {
        resolve_servers = &scheduler->ipv6_resolve_servers;
    } else {
        resolve_servers = &scheduler->ipv4_resolve_servers;
    }
    httpdns_resolve_server_t *resolve_server = httpdns_list_min(resolve_servers,DATA_CMP_FUNC(compare_httpdns_resolve_server));
    *resolve_server_ptr = sdsnew(resolve_server->server);
}

httpdns_resolve_server_t *create_httpdns_resolve_server(char *server) {
    httpdns_resolve_server_t *resolver = (httpdns_resolve_server_t *) malloc(sizeof(httpdns_resolve_server_t));
    memset(resolver, 0, sizeof(httpdns_resolve_server_t));
    resolver->server = sdsnew(server);
    resolver->response_time_ms = DEFAULT_RESOLVE_SERVER_RT;
    return resolver;
}

void destroy_httpdns_resolve_server(httpdns_resolve_server_t *resolve_server) {
    if (NULL == resolve_server) {
        return;
    }
    if (IS_NOT_BLANK_SDS(resolve_server->server)) {
        sdsfree(resolve_server->server);
    }
    free(resolve_server);
}

int32_t compare_httpdns_resolve_server(httpdns_resolve_server_t *server1, httpdns_resolve_server_t *server2) {
    if (NULL == server1 && NULL == server2) {
        return 0;
    }
    if (NULL == server1 && NULL != server2) {
        return -1;
    }
    if (NULL != server1 && NULL == server2) {
        return 1;
    }
    return server1->response_time_ms - server2->response_time_ms;
}
