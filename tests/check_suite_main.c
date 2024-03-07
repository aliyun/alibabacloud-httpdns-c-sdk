

#include <stdlib.h>
#include "check_suit_list.h"
#include "httpdns_log.h"

int main(void) {
    httpdns_log_start();
    CuString *output = CuStringNew();
    CuSuite *suite = CuSuiteNew();

    CuSuiteAddSuite(suite, make_httpdns_time_suite());
    CuSuiteAddSuite(suite, make_httpdns_sign_suite());
    CuSuiteAddSuite(suite, make_httpdns_net_suite());
    CuSuiteAddSuite(suite, make_httpdns_list_suite());
    CuSuiteAddSuite(suite, make_httpdns_response_suite());
    CuSuiteAddSuite(suite, make_httpdns_config_suite());
    CuSuiteAddSuite(suite, make_httpdns_http_suite());
    CuSuiteAddSuite(suite, make_httpdns_scheduler_suite());
    CuSuiteAddSuite(suite, make_httpdns_cache_suite());
    CuSuiteAddSuite(suite, make_httpdns_resolver_suite());
    CuSuiteAddSuite(suite, make_httpdns_client_suite());
    CuSuiteAddSuite(suite, make_httpdns_client_wrapper_suite());
    CuSuiteAddSuite(suite, make_httpdns_localdns_suite());


    CuSuiteRun(suite);

    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
    CuStringDelete(output);
    httpdns_log_stop();
    return EXIT_SUCCESS;
}
