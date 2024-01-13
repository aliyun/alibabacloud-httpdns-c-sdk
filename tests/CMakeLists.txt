project (alicloud_httpdns_c_sdk_test)
include_directories(${CMAKE_SOURCE_DIR})


set(HELLO_WORLD_SOURCE_FILES hello_world.c)


include_directories (${CURL_INCLUDE_DIR})
include_directories ("${CMAKE_SOURCE_DIR}/src")

find_library(CURL_LIBRARY curl)
find_library(PTHREAD_LIBRARY pthread)
find_library(RT_LIBRARY rt)
find_library(M_LIBRARY m)
find_library(SSL_LIBRARY ssl)
find_library(CRYPTO_LIBRARY crypto)
find_library(ZLIB_LIBRARY z)
find_library(DL_LIBRARY dl)

function(_TARGET_TEST_LIBRARIES TEST_BIN_NAME SOURCE_FILES)
    add_executable(${TEST_BIN_NAME} ${SOURCE_FILES})
    target_link_libraries(${TEST_BIN_NAME} alicloud_httpdns_c_sdk_static)
    target_link_libraries(${TEST_BIN_NAME} ${CURL_LIBRARY})
    target_link_libraries(${TEST_BIN_NAME} ${PTHREAD_LIBRARY})
    target_link_libraries(${TEST_BIN_NAME} ${M_LIBRARY})
    target_link_libraries(${TEST_BIN_NAME} ${DL_LIBRARY})
    target_link_libraries(${TEST_BIN_NAME} ${SSL_LIBRARY})
    target_link_libraries(${TEST_BIN_NAME} ${CRYPTO_LIBRARY})
    target_link_libraries(${TEST_BIN_NAME} ${ZLIB_LIBRARY})
    add_test(NAME ${TEST_BIN_NAME}_test COMMAND ${TEST_BIN_NAME})
endfunction()


_TARGET_TEST_LIBRARIES(hello_world "${HELLO_WORLD_SOURCE_FILES}")