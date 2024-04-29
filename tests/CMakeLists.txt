cmake_minimum_required(VERSION 3.0)
project(alicloud_httpdns_c_sdk_test LANGUAGES C)

##################################### 相关依赖库加载 ##########################################
include_directories(${CURL_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CMAKE_INSTALL_PREFIX}/include)
include_directories(${APR_INCLUDE_DIR})
include_directories(${APR_UTIL_INCLUDE_DIR})

##################################### Unit test Application ##########################################
include_directories("${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/tests")
aux_source_directory(${CMAKE_SOURCE_DIR}/tests TEST_SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src SDK_SRC_LIST)

set(TEST_BIN_NAME httpdns_test)

add_executable(${TEST_BIN_NAME} ${TEST_SRC_LIST} ${SDK_SRC_LIST})
target_link_libraries(${TEST_BIN_NAME} ${CURL_LIBRARY})
if (CMAKE_HOST_WIN32)
    target_link_libraries(${TEST_BIN_NAME} ${CRYPTO_LIBRARIES})
    target_link_libraries(${TEST_BIN_NAME} ${SSL_LIBRARIES})
else ()
    target_link_libraries(${TEST_BIN_NAME} ${OPENSSL_LIBRARIES})
    target_link_libraries(${TEST_BIN_NAME} Threads::Threads)
endif ()
target_link_libraries(${TEST_BIN_NAME} ${APR_LIBRARIES})
target_link_libraries(${TEST_BIN_NAME} ${APU_LIBRARIES})

## 这里没有使用原生的CTest，而是使用CuTest框架
add_custom_target(httpdns_unite_test
        COMMAND ${TEST_BIN_NAME}
        DEPENDS ${TEST_BIN_NAME}
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMENT "Running the test executable..."
)


