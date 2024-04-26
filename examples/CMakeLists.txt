cmake_minimum_required(VERSION 3.0)
project(alicloud_httpdns_c_sdk_examples LANGUAGES C CXX)

####################################  编译选项  ###########################################
# default Release C / CXX flags
set(CMAKE_C_FLAGS " -O3 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS " -O3 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_DEBUG " -g -fsanitize=undefined,address -fno-omit-frame-pointer -ggdb -O0 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_DEBUG " -g -fsanitize=undefined,address -fno-omit-frame-pointer -ggdb -O0 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")

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

##################################### 相关依赖库加载 ##########################################
## 三方库
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)


FIND_PROGRAM(APR_CONFIG_BIN NAMES apr-config apr-1-config PATHS /usr/bin /usr/local/bin /usr/local/apr/bin/ /opt/homebrew/Cellar/apr/1.7.4/bin/)
FIND_PROGRAM(APU_CONFIG_BIN NAMES apu-config apu-1-config PATHS /usr/bin /usr/local/bin /usr/local/apr/bin/ /opt/homebrew/Cellar/apr-util/1.6.3_1/bin/)

IF (APR_CONFIG_BIN)
    EXECUTE_PROCESS(
            COMMAND ${APR_CONFIG_BIN} --includedir
            OUTPUT_VARIABLE APR_INCLUDE_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
            COMMAND ${APR_CONFIG_BIN} --cflags
            OUTPUT_VARIABLE APR_C_FLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
            COMMAND ${APR_CONFIG_BIN} --link-ld
            OUTPUT_VARIABLE APR_LIBRARIES
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(STRIP ${APR_LIBRARIES} APR_LIBRARIES)
ELSE ()
    MESSAGE(FATAL_ERROR "Could not find apr-config/apr-1-config")
ENDIF ()

IF (APU_CONFIG_BIN)
    EXECUTE_PROCESS(
            COMMAND ${APU_CONFIG_BIN} --includedir
            OUTPUT_VARIABLE APR_UTIL_INCLUDE_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
            COMMAND ${APU_CONFIG_BIN} --cflags
            OUTPUT_VARIABLE APU_C_FLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
            COMMAND ${APU_CONFIG_BIN} --link-ld
            OUTPUT_VARIABLE APU_LIBRARIES
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(STRIP ${APU_LIBRARIES} APU_LIBRARIES)
ELSE ()
    MESSAGE(FATAL_ERROR "Could not find apu-config/apu-1-config")
ENDIF ()


include_directories(${CURL_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${APR_INCLUDE_DIR})
include_directories(${APR_UTIL_INCLUDE_DIR})
## HTTPDNS SDK
find_library(HTTPDNS_LIBRARY httpdns_c_sdk_static)
include_directories(${CMAKE_INSTALL_PREFIX}/include/httpdns)

##################################### Example Application ################################
function(_TARGET_EXAMPLE_LIBRARIES EXAMPLE_BIN_NAME SOURCE_FILES)
    add_executable(${EXAMPLE_BIN_NAME} ${SOURCE_FILES})
    target_link_libraries(${EXAMPLE_BIN_NAME} ${HTTPDNS_LIBRARY})
    target_link_libraries(${EXAMPLE_BIN_NAME} ${CURL_LIBRARY})
    target_link_libraries(${EXAMPLE_BIN_NAME} ${OPENSSL_LIBRARIES})
    target_link_libraries(${EXAMPLE_BIN_NAME} Threads::Threads)
    target_link_libraries(${EXAMPLE_BIN_NAME} ${APR_LIBRARIES})
    target_link_libraries(${EXAMPLE_BIN_NAME} ${APU_LIBRARIES})
endfunction()

_TARGET_EXAMPLE_LIBRARIES(sync_client_example sync_client_example.c)

_TARGET_EXAMPLE_LIBRARIES(async_client_cxx_example async_client_cxx_example.cpp)

_TARGET_EXAMPLE_LIBRARIES(async_client_example async_client_example.c)