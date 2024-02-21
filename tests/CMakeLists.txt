cmake_minimum_required(VERSION 3.5)
project(alicloud_httpdns_c_sdk_test)

##################################### 相关依赖库加载 ##########################################
include_directories(${CURL_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CJSON_INCLUDE_DIR})
# check框架
find_library(CHECK_LIBRARY check)
include_directories(${CMAKE_INSTALL_PREFIX}/include)

##################################### Unit test Application ##########################################
include_directories("${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/tests")
aux_source_directory(${CMAKE_SOURCE_DIR}/tests TEST_SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src SDK_SRC_LIST)

set(TEST_BIN_NAME  httpdns_test)

add_executable(${TEST_BIN_NAME} ${TEST_SRC_LIST} ${SDK_SRC_LIST})
target_link_libraries(${TEST_BIN_NAME} ${CHECK_LIBRARY})
target_link_libraries(${TEST_BIN_NAME} ${CURL_LIBRARY})
target_link_libraries(${TEST_BIN_NAME} ${OPENSSL_LIBRARIES})
target_link_libraries(${TEST_BIN_NAME} Threads::Threads)
target_link_libraries(${TEST_BIN_NAME} ${CJSON_LIBRARIES})

## 这里没有使用原生的CTest，而是使用check框架
add_custom_target(test
        COMMAND ${TEST_BIN_NAME}
        DEPENDS ${TEST_BIN_NAME}
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMENT "Running the test executable..."
)


