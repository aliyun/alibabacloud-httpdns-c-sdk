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

##################################### C语言标准 ##########################################
# Follow ISO C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)          # Use GNU extensions and POSIX standard

################################# 动态编译配置 ############################################
# 配置日志等级
if (DEFINED HTTPDNS_LOG_LEVEL)
    set(HTTPDNS_LOG_LEVEL ${HTTPDNS_LOG_LEVEL})
else ()
    set(HTTPDNS_LOG_LEVEL HTTPDNS_LOG_INFO)
endif ()
# 配置日志文件路径
if (DEFINED HTTPDNS_LOG_FILE_PATH)
    set(HTTPDNS_LOG_FILE_PATH ${HTTPDNS_LOG_FILE_PATH})
else ()
    set(HTTPDNS_LOG_FILE_PATH "/tmp/httpdns.log")
endif ()
# 配置HTTPDNS集群
if (DEFINED HTTPDNS_REGION)
    set(HTTPDNS_REGION ${HTTPDNS_REGION})
else ()
    set(HTTPDNS_REGION "cn")
endif ()

# 配置HTTPDNS失败重试次数
if (DEFINED HTTPDNS_RETRY_TIMES)
    set(HTTPDNS_RETRY_TIMES ${HTTPDNS_RETRY_TIMES})
else ()
    set(HTTPDNS_RETRY_TIMES 1)
endif ()


configure_file(
        "${CMAKE_SOURCE_DIR}/configuration.h.in"
        "${CMAKE_SOURCE_DIR}/src/httpdns_env_configuration.h"
)

################################# 相关依赖库加载 ############################################
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

include_directories(${CURL_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})

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
if(NOT TARGET uninstall)
    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
            IMMEDIATE @ONLY)

    add_custom_target(uninstall
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
################################# 单元测试 ############################################
enable_testing()

add_subdirectory(tests)