//
// Created by cagaoshuai on 2024/1/25.
//

#ifndef HTTPDNS_C_SDK_CHECK_SUIT_LIST_H
#define HTTPDNS_C_SDK_CHECK_SUIT_LIST_H

#include<check.h>
#include<stdbool.h>
#include "log.h"

Suite *make_httpdns_time_suite(void);

Suite *make_httpdns_sign_suite(void);

Suite *make_httpdns_net_suite(void);

Suite *make_httpdns_list_suite(void);

Suite *make_httpdns_response_suite(void);

Suite *make_httpdns_config_suite(void);

Suite *make_httpdns_http_suite(void);

Suite *make_httpdns_scheduler_suite(void);

Suite *make_httpdns_cache_suite(void);

Suite *make_httpdns_resolver_suite(void);

Suite *make_httpdns_client_suite(void);

Suite *make_httpdns_client_wrapper_suite(void);

#endif //HTTPDNS_C_SDK_CHECK_SUIT_LIST_H
