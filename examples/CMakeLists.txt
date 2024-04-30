project(alicloud_httpdns_c_sdk_examples LANGUAGES C CXX)

##################################### Example Application ################################
include_directories(${CURL_INCLUDE_DIR})
include_directories(${APU_INCLUDE_DIR})
include_directories(${APR_INCLUDE_DIR})
include_directories(${CJSON_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(../src)

function(_TARGET_EXAMPLE_LIBRARIES EXAMPLE_BIN_NAME SOURCE_FILES)
    add_executable(${EXAMPLE_BIN_NAME} ${SOURCE_FILES})
    target_link_libraries(${EXAMPLE_BIN_NAME} PRIVATE ${CMAKE_PROJECT_NAME}_static ${OPENSSL_LIBRARIES} ${CURL_LIBRARIES} ${CJSON_LIBRARIES} ${APU_LIBRARIES} ${APR_LIBRARIES})
endfunction()

_TARGET_EXAMPLE_LIBRARIES(sync_client_example sync_client_example.c)

_TARGET_EXAMPLE_LIBRARIES(async_client_cxx_example async_client_cxx_example.cpp)

_TARGET_EXAMPLE_LIBRARIES(async_client_example async_client_example.c)