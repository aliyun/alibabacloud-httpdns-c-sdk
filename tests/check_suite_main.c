/*
 * Check: a unit test framework for C
 * Copyright (C) 2001, 2002 Arien Malec
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */



#include <stdlib.h>
#include <check.h>
#include "check_suit_list.h"
#include "httpdns_global_config.h"

int main(void) {
    init_httpdns_sdk();
    int number_failed;
    SRunner *suite_runner = srunner_create(make_httpdns_time_suite());
    srunner_add_suite(suite_runner, make_httpdns_sign_suite());
    srunner_add_suite(suite_runner, make_httpdns_net_suite());
    srunner_add_suite(suite_runner, make_httpdns_list_suite());
    srunner_add_suite(suite_runner, make_httpdns_response_suite());
    srunner_add_suite(suite_runner, make_httpdns_config_suite());
    srunner_add_suite(suite_runner, make_httpdns_http_suite());
    srunner_add_suite(suite_runner, make_httpdns_scheduler_suite());
    srunner_add_suite(suite_runner, make_httpdns_cache_suite());
    // Uncomment the following if you want to debug.
    srunner_set_fork_status(suite_runner, CK_NOFORK);
    srunner_run_all(suite_runner, CK_VERBOSE);
    number_failed = srunner_ntests_failed(suite_runner);
    srunner_free(suite_runner);
    cleanup_httpdns_sdk();
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
