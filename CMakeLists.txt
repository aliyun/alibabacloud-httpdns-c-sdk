cmake_minimum_required(VERSION 3.0)
PROJECT(httpdns_c_sdk LANGUAGES C)

####################################  编译选项  ###########################################
# default Release C flags
set(CMAKE_C_FLAGS " -O3 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_C_FLAGS_DEBUG " -g -ggdb -O0 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_C_FLAGS_RELEASE " -O3 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_C_FLAGS_MINSIZEREF " -Os -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_C_FLAGS_RELWITHDEBINFO " -O2 -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_C_FLAGS_COVERAGE " ${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}/bin)
set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)

option(ADD_LOG_USE_COLOR "ON for enabling color printing" OFF)
IF (ADD_LOG_USE_COLOR)
    add_definitions(-DLOG_USE_COLOR)
ENDIF (ADD_LOG_USE_COLOR)

option(ADD_TEST_DEBUG_LOG "ON for unit test logs" OFF)
IF (ADD_TEST_DEBUG_LOG)
    add_definitions(-DTEST_DEBUG_LOG)
ENDIF (ADD_TEST_DEBUG_LOG)

# cn: china mainland, sg: singapore, hk: hong kong
add_definitions(-DHTTPDNS_REGION="cn")

##################################### C语言标准 ##########################################
# Follow ISO C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)          # Use GNU extensions and POSIX standard

################################# 相关依赖库加载 ############################################
# find dependency librarys
if (CMAKE_HOST_WIN32)
    set(EXTRA_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/third_party/include)
    if (CMAKE_CL_64)
        set(EXTRA_LIB_PATH ${CMAKE_SOURCE_DIR}/third_party/lib/x64/)
    else ()
        set(EXTRA_LIB_PATH ${CMAKE_SOURCE_DIR}/third_party/lib/Win32/)
    endif ()
    find_path(APR_INCLUDE_DIR NAMES apr.h PATH_SUFFIXES apr PATHS ${EXTRA_INCLUDE_PATH})
    find_library(APR_LIBRARY NAMES libapr-1 PATHS ${EXTRA_LIB_PATH})

    find_path(APR_UTIL_INCLUDE_DIR NAMES apu.h PATH_SUFFIXES apu PATHS ${EXTRA_INCLUDE_PATH})
    find_library(APR_UTIL_LIBRARY NAMES libaprutil-1 PATHS ${EXTRA_LIB_PATH})

    find_path(CURL_INCLUDE_DIR NAMES curl/curl.h PATHS ${EXTRA_INCLUDE_PATH})
    find_library(CURL_LIBRARY NAMES libcurl PATHS ${EXTRA_LIB_PATH})

    find_path(CURL_INCLUDE_DIR NAMES cJSON.h PATH_SUFFIXES cjson PATHS ${EXTRA_INCLUDE_PATH})
    find_library(CURL_LIBRARY NAMES libcjson PATHS ${EXTRA_LIB_PATH})

    find_path(CURL_INCLUDE_DIR NAMES openssl/ssl.h PATHS ${EXTRA_INCLUDE_PATH})
    find_library(CURL_LIBRARY NAMES libssl PATHS ${EXTRA_LIB_PATH})

else ()

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
    find_package(CURL REQUIRED)
    find_package(OpenSSL REQUIRED)
endif ()

include_directories(${CURL_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${APR_INCLUDE_DIR})
include_directories(${APR_UTIL_INCLUDE_DIR})

aux_source_directory(src SRC_LIST)

################################# 库文件编译安装 ############################################
## 静态库安装
add_library(${CMAKE_PROJECT_NAME}_static STATIC ${SRC_LIST})
INSTALL(TARGETS ${CMAKE_PROJECT_NAME}_static ARCHIVE DESTINATION lib)

## 动态库安装
IF (APPLE)
    MESSAGE(STATUS "For mac os, skip build dynamic library")
ELSE ()
    add_library(${CMAKE_PROJECT_NAME} SHARED ${SRC_LIST})
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC BUILD_SHARED_LIB)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES VERSION ${CMAKE_VERSION} SOVERSION ${CMAKE_VERSION})
    INSTALL(TARGETS ${CMAKE_PROJECT_NAME} LIBRARY DESTINATION lib)
ENDIF ()

## 头文件安装
file(GLOB HEADER_FILES src/*.h)
INSTALL(FILES
        ${HEADER_FILES}
        DESTINATION include/httpdns)
################################# 库文件编译卸载 ############################################
# uninstall target
if (NOT TARGET uninstall)
    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
            IMMEDIATE @ONLY)

    add_custom_target(uninstall
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif ()
################################# 单元测试 ############################################
#add_subdirectory(tests)