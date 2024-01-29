//
// Created by cagaoshuai on 2024/1/29.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_RESOLVE_REQUEST_H
#define HTTPDNS_C_SDK_HTTPDNS_RESOLVE_REQUEST_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char *host;
    char *account_id;
    char *secret_key;
    char *resolver;
    char *query_type;
    char *client_ip;
    char *sdk_version;
    char *user_agent;
    char *sdns_params;
    bool using_https;
    bool using_sign;
    bool using_multi;
    bool using_cache;
    int32_t timeout_ms;
    char *cache_key;
} httpdns_resolve_request_t;


void httpdns_resolve_request_append_sdns_params(httpdns_resolve_request_t *request, char *key, char *value);

void httpdns_resolve_request_set_host(httpdns_resolve_request_t *request, char *host);

void httpdns_resolve_request_set_account_id(httpdns_resolve_request_t *request, char *account_id);

void httpdns_resolve_request_set_secret_key(httpdns_resolve_request_t *request, char *secret_key);

void httpdns_resolve_request_set_resolver(httpdns_resolve_request_t *request, char *resolver);

void httpdns_resolve_request_set_query_type(httpdns_resolve_request_t *request, char *query_type);

void httpdns_resolve_request_set_client_ip(httpdns_resolve_request_t *request, char *client_ip);

void httpdns_resolve_request_set_sdk_version(httpdns_resolve_request_t *request, char *sdk_version);

void httpdns_resolve_request_set_user_agent(httpdns_resolve_request_t *request, char *user_agent);

void httpdns_resolve_request_set_cache_key(httpdns_resolve_request_t *request, char *cache_key);

void httpdns_resolve_request_set_timeout_ms(httpdns_resolve_request_t *request, int32_t timeout_ms);

void httpdns_resolve_request_set_using_https(httpdns_resolve_request_t *request, bool using_https);

void httpdns_resolve_request_set_using_sign(httpdns_resolve_request_t *request, bool using_sign);

void httpdns_resolve_request_set_using_multi(httpdns_resolve_request_t *request, bool using_multi);

httpdns_resolve_request_t *
httpdns_resolve_request_create(httpdns_config_t *config, char *host, char *resolver, char *query_type);

httpdns_resolve_request_t *httpdns_resolve_request_clone(httpdns_resolve_request_t *origin_resolve_request);

sds httpdns_resolve_request_to_string(httpdns_resolve_request_t *request);

int32_t httpdns_resolve_request_valid(httpdns_resolve_request_t *request);


#endif //HTTPDNS_C_SDK_HTTPDNS_RESOLVE_REQUEST_H
