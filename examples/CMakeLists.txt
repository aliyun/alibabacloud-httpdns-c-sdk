project(alicloud_httpdns_c_sdk_examples LANGUAGES C CXX)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

set(CMAKE_VERSION 2.0.0)
####################################  编译选项  ###########################################
# default Release C / CXX flags
set(CMAKE_C_FLAGS " -O3 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS " -O3 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_DEBUG " -g -ggdb -O0 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_DEBUG " -g -ggdb -O0 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_RELEASE " -O3 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_RELEASE " -O3 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_MINSIZEREF " -Os -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_MINSIZEREF " -Os -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_RELWITHDEBINFO " -O2 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO " -O2 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_COVERAGE " ${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
set(CMAKE_CXX_FLAGS_COVERAGE " ${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverag")

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}/bin)
set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

##################################### C语言标准 ##########################################
# Follow ISO C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)          # Use GNU extensions and POSIX standard

##################################### C语言标准 ##########################################
find_library(HTTPDNS_LIBRARY httpdns_c_sdk_static)
find_library(CURL_LIBRARY curl)
find_library(PTHREAD_LIBRARY pthread)
find_library(SSL_LIBRARY ssl)
find_library(CRYPTO_LIBRARY crypto)

##################################### Example Application ################################
function(_TARGET_EXAMPLE_LIBRARIES EXAMPLE_BIN_NAME SOURCE_FILES)
    add_executable(${EXAMPLE_BIN_NAME}        ${SOURCE_FILES})
    target_link_libraries(${EXAMPLE_BIN_NAME} ${HTTPDNS_LIBRARY})
    target_link_libraries(${EXAMPLE_BIN_NAME} ${CURL_LIBRARY})
    target_link_libraries(${EXAMPLE_BIN_NAME} ${PTHREAD_LIBRARY})
    target_link_libraries(${EXAMPLE_BIN_NAME} ${SSL_LIBRARY})
    target_link_libraries(${EXAMPLE_BIN_NAME} ${CRYPTO_LIBRARY})
endfunction()

_TARGET_EXAMPLE_LIBRARIES(sync_client_example sync_client_example.c)

_TARGET_EXAMPLE_LIBRARIES(sync_client_cxx_example sync_client_cxx_example.cpp)

_TARGET_EXAMPLE_LIBRARIES(async_client_example async_client_example.c)

_TARGET_EXAMPLE_LIBRARIES(sync_server_example sync_server_example.c)