project(alicloud_httpdns_c_sdk_examples)

include_directories("${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/examples")



find_library(CURL_LIBRARY curl)
find_library(PTHREAD_LIBRARY pthread)
find_library(SSL_LIBRARY ssl)
find_library(CRYPTO_LIBRARY crypto)




function(_TARGET_EXAMPLE_LIBRARIES EXAMPLE_BIN_NAME SOURCE_FILES)
    add_executable(${EXAMPLE_BIN_NAME} ${SOURCE_FILES})
    target_link_libraries(${EXAMPLE_BIN_NAME} ${CMAKE_PROJECT_NAME}_static)
    target_link_libraries(${EXAMPLE_BIN_NAME} ${CURL_LIBRARY})
    target_link_libraries(${EXAMPLE_BIN_NAME} ${PTHREAD_LIBRARY})
    target_link_libraries(${EXAMPLE_BIN_NAME} ${SSL_LIBRARY})
    target_link_libraries(${EXAMPLE_BIN_NAME} ${CRYPTO_LIBRARY})
endfunction()

_TARGET_EXAMPLE_LIBRARIES(sync_client_in_linux_example sync_client_in_linux_example.c)

_TARGET_EXAMPLE_LIBRARIES(async_client_in_linux_example async_client_in_linux_example.c)