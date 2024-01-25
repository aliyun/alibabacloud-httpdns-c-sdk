project(alicloud_httpdns_c_sdk_test)

include_directories("${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/tests")

aux_source_directory(${CMAKE_SOURCE_DIR}/tests TEST_SRC_LIST)

find_library(CHECK_LIBRARY check)

set(TEST_BIN_NAME  httpdns_test)

add_executable(${TEST_BIN_NAME} ${TEST_SRC_LIST})
target_link_libraries(${TEST_BIN_NAME} ${CMAKE_PROJECT_NAME}_static)
target_link_libraries(${TEST_BIN_NAME} ${CHECK_LIBRARY})
add_test(NAME ${TEST_BIN_NAME}_test COMMAND ${TEST_BIN_NAME})


