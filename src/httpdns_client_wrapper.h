//
// Created by cagaoshuai on 2024/1/31.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_CLIENT_WRAPPER_H
#define HTTPDNS_C_SDK_HTTPDNS_CLIENT_WRAPPER_H

#include  "httpdns_resolve_result.h"
#include "httpdns_client.h"

int32_t httpdns_client_simple_resolve(httpdns_client_t *httpdns_client,
                                      char *host,
                                      char *query_type,
                                      char *client_ip,
                                      httpdns_resolve_result_t **result);


#endif //HTTPDNS_C_SDK_HTTPDNS_CLIENT_WRAPPER_H
