project(alicloud_httpdns_c_sdk_test LANGUAGES C)

##################################### Unit test Application ##########################################
include_directories(${CURL_INCLUDE_DIR})
include_directories(${APU_INCLUDE_DIR})
include_directories(${APR_INCLUDE_DIR})
include_directories(${CJSON_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories("${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/tests")
aux_source_directory(${CMAKE_SOURCE_DIR}/tests TEST_SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src SDK_SRC_LIST)

set(TEST_BIN_NAME hdns_test)

add_executable(${TEST_BIN_NAME} ${TEST_SRC_LIST} ${SDK_SRC_LIST})
target_link_libraries(${TEST_BIN_NAME} PRIVATE ${OPENSSL_LIBRARIES} ${CURL_LIBRARIES} ${CJSON_LIBRARIES} ${APU_LIBRARIES} ${APR_LIBRARIES})

## 这里没有使用原生的CTest，而是使用CuTest框架
add_custom_target(httpdns_unite_test
        COMMAND ${TEST_BIN_NAME}
        DEPENDS ${TEST_BIN_NAME}
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMENT "Running the test executable..."
)


