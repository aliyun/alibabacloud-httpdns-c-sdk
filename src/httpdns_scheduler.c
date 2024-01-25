//
// Created by cagaoshuai on 2024/1/11.
//
#include "httpdns_scheduler.h"
#include "httpdns_memory.h"
#include "httpdns_response.h"
#include "httpdns_ip.h"
#include "httpdns_sign.h"

httpdns_scheduler_t *httpdns_scheduler_create(httpdns_config_t *config) {
    if (httpdns_config_is_valid(config) != HTTPDNS_SUCCESS) {
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(scheduler, httpdns_scheduler_t);
    httpdns_list_init(&scheduler->ipv4_resolve_servers);
    httpdns_list_init(&scheduler->ipv6_resolve_servers);
    scheduler->config = config;
    return scheduler;
}

void httpdns_scheduler_print(httpdns_scheduler_t *scheduler) {
    if (NULL == scheduler) {
        printf("httpdns_scheduler_t()");
        return;
    }
    printf("httpdns_scheduler_t(ipv4_resolve_servers=");
    httpdns_list_print(&scheduler->ipv4_resolve_servers, DATA_PRINT_FUNC(httpdns_ip_print));
    printf(",ipv6_resolve_servers=");
    httpdns_list_print(&scheduler->ipv6_resolve_servers, DATA_PRINT_FUNC(httpdns_ip_print));
    printf(")");
}

void httpdns_scheduler_destroy(httpdns_scheduler_t *scheduler) {
    if (NULL == scheduler) {
        return;
    }
    httpdns_list_free(&scheduler->ipv4_resolve_servers, DATA_FREE_FUNC(httpdns_ip_destroy));
    httpdns_list_free(&scheduler->ipv6_resolve_servers, DATA_FREE_FUNC(httpdns_ip_destroy));
    free(scheduler);
}


static void generate_nonce(char *nonce, size_t size) {
    for (int i = 0; i < size; i++) {
        int rand_val = (int) random() % 16;
        sprintf(&nonce[i], "%x", rand_val);
    }
    nonce[size] = '\0';
}

static void httpdns_parse_body(void *response_body, httpdns_scheduler_t *scheduler) {
    httpdns_schedule_response_t *schedule_response = httpdns_response_parse_schedule(response_body);
    if (NULL != schedule_response) {
        httpdns_list_dup(&scheduler->ipv4_resolve_servers, &schedule_response->service_ip,
                         DATA_CLONE_FUNC(httpdns_ip_create));
        httpdns_list_dup(&scheduler->ipv6_resolve_servers, &schedule_response->service_ipv6,
                         DATA_CLONE_FUNC(httpdns_ip_create));
        httpdns_schedule_response_destroy(schedule_response);
    }
}

int32_t httpdns_scheduler_refresh(httpdns_scheduler_t *scheduler) {
    if (NULL == scheduler || NULL == scheduler->config) {
        return HTTPDNS_PARAMETER_ERROR;
    }
    net_stack_type_t net_stack_type = httpdns_net_stack_type_get(scheduler->net_stack_detector);
    httpdns_config_t *config = scheduler->config;
    struct list_head *boot_servers;
    if (IPV6_ONLY == net_stack_type) {
        boot_servers = &config->ipv6_boot_servers;
    } else {
        boot_servers = &config->ipv4_boot_servers;
    }
    size_t boot_server_num = httpdns_list_size(boot_servers);
    if (boot_server_num <= 0) {
        return HTTPDNS_BOOT_SERVER_EMPTY;
    }
    const char *http_scheme = config->using_https ? HTTPS_SCHEME : HTTP_SCHEME;
    for (int i = 0; i < boot_server_num; i++) {
        sds url = sdsnew(http_scheme);
        url = sdscat(url, httpdns_list_get(boot_servers, i));
        url = sdscat(url, "/");
        url = sdscat(url, config->account_id);
        url = sdscat(url, "/ss?platform=linux&sdkVersion=");
        url = sdscat(url, config->sdk_version);
        url = sdscat(url, "&region=");
        url = sdscat(url, config->region);
        if (config->using_sign && NULL != config->secret_key) {
            char nonce[SCHEDULE_NONCE_SIZE + 1];
            generate_nonce(nonce, SCHEDULE_NONCE_SIZE);
            httpdns_signature_t *signature = httpdns_signature_create(nonce,
                                                                      config->secret_key,
                                                                      MAX_SCHEDULE_SIGNATURE_OFFSET_TIME,
                                                                      httpdns_time_now());
            url = sdscat(url, "&s=");
            url = sdscat(url, signature->sign);
            url = sdscat(url, "&t=");
            url = sdscat(url, signature->timestamp);
            url = sdscat(url, "&n=");
            url = sdscat(url, nonce);
            destroy_httpdns_signature(signature);
        }
        httpdns_http_context_t *http_context = httpdns_http_context_create(url, config->timeout_ms);
        sdsfree(url);
        httpdns_http_single_exchange(http_context);
        printf("\n");
        httpdns_http_context_print(http_context);
        bool success = (http_context->response_status == HTTP_STATUS_OK);
        if (success) {
            httpdns_parse_body(http_context->response_body, scheduler);
        } else {
            printf("response_body=%s, response_status=%d, response_rt=%d",
                   http_context->response_body,
                   http_context->response_status,
                   http_context->response_rt_ms
            );
        }
        httpdns_http_context_destroy(http_context);
        if (success) {
            return HTTPDNS_SUCCESS;
        }
    }
    return HTTPDNS_FAILURE;
}


static int32_t update_server_rt(int32_t old_rt_val, int32_t new_rt_val) {
    if (old_rt_val == DEFAULT_IP_RT) {
        return new_rt_val;
    }
    if (new_rt_val * DELTA_WEIGHT_UPDATE_RATION > old_rt_val) {
        return old_rt_val;
    }
    return (int32_t) (new_rt_val * DELTA_WEIGHT_UPDATE_RATION + old_rt_val * (1.0 - DELTA_WEIGHT_UPDATE_RATION));
}

void httpdns_scheduler_update(httpdns_scheduler_t *scheduler, char *server, int32_t rt) {
    if (IS_BLANK_STRING(server) || NULL == scheduler || rt <= 0) {
        return;
    }
    httpdns_ip_t *resolve_server = httpdns_list_search(&scheduler->ipv4_resolve_servers, server,
                                                       DATA_SEARCH_FUNC(httpdns_ip_search));
    if (NULL == resolve_server) {
        resolve_server = httpdns_list_search(&scheduler->ipv6_resolve_servers, server,
                                             DATA_SEARCH_FUNC(httpdns_ip_search));
    }
    if (NULL != resolve_server) {
        resolve_server->rt = update_server_rt(resolve_server->rt, rt);
        return;
    }
}

char *httpdns_scheduler_get(httpdns_scheduler_t *scheduler) {
    if (NULL == scheduler) {
        return NULL;
    }
    net_stack_type_t net_stack_type = httpdns_net_stack_type_get(scheduler->net_stack_detector);
    struct list_head *resolve_servers;
    if (IPV6_ONLY == net_stack_type) {
        resolve_servers = &scheduler->ipv6_resolve_servers;
    } else {
        resolve_servers = &scheduler->ipv4_resolve_servers;
    }
    httpdns_ip_t *resolve_server = httpdns_list_min(resolve_servers, DATA_CMP_FUNC(httpdns_ip_cmp));
    if (NULL != resolve_server) {
        return sdsnew(resolve_server->ip);
    }
    return NULL;
}

void
httpdns_scheduler_set_net_stack_detector(httpdns_scheduler_t *scheduler, httpdns_net_stack_detector_t *net_stack_detector) {
    if (NULL != scheduler && NULL != net_stack_detector) {
        scheduler->net_stack_detector = net_stack_detector;
    }
}