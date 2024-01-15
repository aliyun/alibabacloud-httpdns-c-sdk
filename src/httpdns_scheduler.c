//
// Created by cagaoshuai on 2024/1/11.
//
#include "httpdns_scheduler.h"

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
    httpdns_list_free(&scheduler->ipv4_resolve_servers, (data_free_function_ptr_t) sdsfree);
    httpdns_list_free(&scheduler->ipv6_resolve_servers, (data_free_function_ptr_t) sdsfree);
}

static httpdns_resolve_server_t *_clone_resolve_server(httpdns_resolve_server_t *resolve_server) {
    if (NULL == resolve_server) {
        return NULL;
    }
    return create_httpdns_resolve_server(resolve_server->server);
}

static httpdns_resolve_server_t *clone_httpdns_resolve_server(const httpdns_resolve_server_t *origin_resolver) {
    if (NULL == origin_resolver) {
        return NULL;
    }
    httpdns_resolve_server_t *resolver = (httpdns_resolve_server_t *) malloc(sizeof(httpdns_resolve_server_t));
    if (NULL != origin_resolver->server) {
        resolver->server = sdsdup(origin_resolver->server);
    }
    resolver->weight = origin_resolver->weight;
    return resolver;
}

static void _httpdns_parse_resolvers(cJSON *c_json_body, const char *item_name, struct list_head *dst_resolvers) {
    cJSON *resolvers = cJSON_GetObjectItem(c_json_body, item_name);
    if (NULL != resolvers) {
        size_t resolve_num = cJSON_GetArraySize(resolvers);
        struct list_head resolve_list;
        httpdns_list_init(&resolve_list);
        for (int i = 0; i < resolve_num; i++) {
            cJSON *ipv4_resolver = cJSON_GetArrayItem(resolvers, i);
            httpdns_resolve_server_t *resolver = create_httpdns_resolve_server(ipv4_resolver->valuestring);
            httpdns_list_add(&resolve_list, resolver, (data_clone_function_ptr_t)clone_httpdns_resolve_server);
        }
        if (httpdns_list_size(&resolve_list) > 0) {
            httpdns_list_free(dst_resolvers, (data_free_function_ptr_t) destroy_httpdns_resolve_server);
            httpdns_list_dup(dst_resolvers, &resolve_list, (data_clone_function_ptr_t) _clone_resolve_server);
            httpdns_list_shuffle(dst_resolvers);
        }
    }
}

static void _httpdns_parse_body(void *response_body, httpdns_scheduler_t *scheduler) {
    cJSON *c_json_body = cJSON_Parse(response_body);
    if (NULL == c_json_body) {
        return;
    }
    _httpdns_parse_resolvers(c_json_body, JSON_BODY_IPV4_RESOLVERS_ITEM, &scheduler->ipv4_resolve_servers);
    _httpdns_parse_resolvers(c_json_body, JSON_BODY_IPV6_RESOLVERS_ITEM, &scheduler->ipv6_resolve_servers);
}

int32_t httpdns_scheduler_refresh_resolve_servers(httpdns_scheduler_t *scheduler) {
    if (NULL == scheduler || NULL == scheduler->config) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    u_int32_t net_stack_type = get_net_stack_type(scheduler->net_stack_detector);
    httpdns_config_t *config = scheduler->config;
    struct list_head boot_servers = (IPV6_ONLY == net_stack_type) ? config->ipv6_boot_servers
                                                                  : config->ipv4_boot_servers;
    size_t boot_server_num = httpdns_list_size(&boot_servers);
    if (boot_server_num <= 0) {
        return HTTPDNS_FAILURE;
    }
    const char *http_scheme = config->using_https ? HTTPS_SCHEME : HTTP_SCHEME;
    for (int i = 0; i < boot_server_num; i++) {
        httpdns_list_node_t *node = httpdns_list_get(&boot_servers, i);
        sds url = sdsnew(http_scheme);
        url = sdscat(url, node->data);
        url = sdscat(url, "/");
        url = sdscat(url, config->account_id);
        url = sdscat(url, "/ss?platform=linux&sdk_version=");
        url = sdscat(url, config->sdk_version);
        httpdns_http_request_t *request = create_httpdns_http_request(url, config->timeout_ms, NULL);
        httpdns_http_response_t *response = httpdns_http_single_request_exchange(request);
        if (response->http_status == HTTP_STATUS_OK) {
            _httpdns_parse_body(response->body, scheduler);
        }
        destroy_httpdns_http_request(request);
        destroy_httpdns_http_response(response);
    }
    return HTTPDNS_SUCCESS;
}

void httpdns_scheduler_get_resolve_server(httpdns_scheduler_t *scheduler, char **resolve_server_ptr) {
    if (NULL == scheduler) {
        return;
    }
    u_int32_t net_stack_type = get_net_stack_type(scheduler->net_stack_detector);
    struct list_head resolve_servers = (IPV6_ONLY == net_stack_type) ? scheduler->ipv6_resolve_servers
                                                                     : scheduler->ipv4_resolve_servers;
    size_t resolve_server_num = httpdns_list_size(&resolve_servers);
    int max_weight_resolve_server_index = 0;
    int max_weight = INT32_MIN;
    for (int i = 0; i < resolve_server_num; i++) {
        httpdns_resolve_server_t *resolver = httpdns_list_get(&resolve_servers, i);
        if (resolver->weight == DEFAULT_RESOLVER_WEIGHT) {
            max_weight_resolve_server_index = i;
            break;
        }
        if (resolver->weight > max_weight) {
            max_weight = resolver->weight;
            max_weight_resolve_server_index = i;
        }
    }
    httpdns_resolve_server_t *resolve_server = httpdns_list_get(&resolve_servers, max_weight_resolve_server_index);
    *resolve_server_ptr = sdsdup(resolve_server->server);
}

httpdns_resolve_server_t *create_httpdns_resolve_server(char *server) {
    httpdns_resolve_server_t *resolver = (httpdns_resolve_server_t *) malloc(sizeof(httpdns_resolve_server_t));
    resolver->server = sdsnew(server);
    resolver->weight = DEFAULT_RESOLVER_WEIGHT;
    return resolver;
}

void destroy_httpdns_resolve_server(httpdns_resolve_server_t *resolve_server) {
    if (NULL == resolve_server) {
        return;
    }
    if (NULL != resolve_server->server) {
        sdsfree(resolve_server->server);
    }
}

