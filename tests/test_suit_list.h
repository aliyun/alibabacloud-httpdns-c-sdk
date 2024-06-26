//
// Created by caogaoshuai on 2024/1/25.
//

#ifndef HDNS_C_SDK_TEST_SUIT_LIST_H
#define HDNS_C_SDK_TEST_SUIT_LIST_H

#include "CuTest.h"
#include<stdbool.h>
#include "hdns_log.h"
#include "hdns_api.h"

#define HDNS_TEST_SECRET_KEY    NULL
#define HDNS_TEST_ACCOUNT       "139450"

void add_hdns_transport_tests(CuSuite *suite);

void add_hdns_session_tests(CuSuite *suite);

void add_hdns_scheduler_tests(CuSuite *suite);

void add_hdns_net_tests(CuSuite *suite);

void add_hdns_list_tests(CuSuite *suite);

void add_hdns_http_tests(CuSuite *suite);

void add_hdns_config_tests(CuSuite *suite);

void add_hdns_cache_tests(CuSuite *suite);

void add_hdns_api_tests(CuSuite *suite);

void add_hdns_thread_safe_tests(CuSuite *suite);

void add_hdns_file_tests(CuSuite *suite);

void add_hdns_utils_tests(CuSuite *suite);


#endif
