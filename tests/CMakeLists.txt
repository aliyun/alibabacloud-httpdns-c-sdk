project(alicloud_httpdns_c_sdk_test)

include_directories("${CMAKE_SOURCE_DIR}/src")

function(_TARGET_TEST_LIBRARIES TEST_BIN_NAME SOURCE_FILES)
    add_executable(${TEST_BIN_NAME} ${SOURCE_FILES})
    target_link_libraries(${TEST_BIN_NAME} ${CMAKE_PROJECT_NAME}_static)
    add_test(NAME ${TEST_BIN_NAME}_test COMMAND ${TEST_BIN_NAME})
endfunction()

_TARGET_TEST_LIBRARIES(net_stack_detector net_stack_detector_test.c)

_TARGET_TEST_LIBRARIES(httpdns_list httpdns_list_test.c)

_TARGET_TEST_LIBRARIES(httpdns_config httpdns_config_test.c)

_TARGET_TEST_LIBRARIES(httpdns_http httpdns_http_test.c)

_TARGET_TEST_LIBRARIES(httpdns_scheduler httpdns_scheduler_test.c)

_TARGET_TEST_LIBRARIES(httpdns_sign httpdns_sign_test.c)

_TARGET_TEST_LIBRARIES(response_parser response_parser_test.c)

_TARGET_TEST_LIBRARIES(httpdns_resolver httpdns_resolver_test.c)


_TARGET_TEST_LIBRARIES(httpdns_time httpdns_time_test.c)