

#include "test_suit_list.h"
#include "hdns_log.h"

int main(void) {



    CuString *output = CuStringNew();
    CuSuite *suite = CuSuiteNew();

//    add_hdns_transport_tests(suite);
//    add_hdns_session_tests(suite);
//    add_hdns_scheduler_tests(suite);
//    add_hdns_net_tests(suite);
//    add_hdns_list_tests(suite);
//    add_hdns_http_tests(suite);
//    add_hdns_config_tests(suite);
//    add_hdns_cache_tests(suite);
//    add_hdns_api_tests(suite);
//    add_hdns_thread_safe_tests(suite);
    add_hdns_file_tests(suite);

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);

    CuStringDelete(output);
    int exit_code = suite->failCount > 0 ? 1 : 0;
    CuSuiteDelete(suite);
    return exit_code;
}
