PROJECT(httpdns_c_sdk)

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

################################# 动态编译配置 ############################################
# 配置日志等级
if (DEFINED HTTPDNS_LOG_LEVEL)
    set(HTTPDNS_LOG_LEVEL ${HTTPDNS_LOG_LEVEL})
else ()
    set(HTTPDNS_LOG_LEVEL LOG_INFO)
endif ()
# 配置日志文件路径
if (DEFINED LOG_FILE_PATH)
    set(LOG_FILE_PATH ${LOG_FILE_PATH})
else ()
    set(LOG_FILE_PATH "/tmp/httpdns.log")
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
        "${CMAKE_SOURCE_DIR}/src/configuration.h"
)

################################# 相关依赖库加载 ############################################
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

include_directories(${CURL_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})

aux_source_directory(src SRC_LIST)
################################# 静态库编译安装 ############################################
add_library(${CMAKE_PROJECT_NAME}_static STATIC ${SRC_LIST})
target_link_libraries(${CMAKE_PROJECT_NAME}_static ${OPENSSL_LIBRARIES})
target_link_libraries(${CMAKE_PROJECT_NAME}_static ${CURL_LIBRARY})
target_link_libraries(${CMAKE_PROJECT_NAME}_static Threads::Threads)

INSTALL(TARGETS ${CMAKE_PROJECT_NAME}_static ARCHIVE DESTINATION lib)



IF (APPLE)
    MESSAGE(STATUS "For mac os, skip build dynamic library")
ELSE ()
    add_library(${CMAKE_PROJECT_NAME} SHARED ${SRC_LIST})
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC BUILD_SHARED_LIB)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES VERSION ${CMAKE_VERSION} SOVERSION ${CMAKE_VERSION})
    INSTALL(TARGETS ${CMAKE_PROJECT_NAME} LIBRARY DESTINATION lib)
ENDIF ()


file(GLOB HEADER_FILES src/*.h)

INSTALL(FILES
        ${HEADER_FILES}
        DESTINATION include/${CMAKE_PROJECT_NAME})

enable_testing()
add_subdirectory(tests)

add_subdirectory(examples)
