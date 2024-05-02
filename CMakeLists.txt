cmake_minimum_required(VERSION 3.0)

if (DEFINED VCPKG_ROOT)
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
else ()
    if (CMAKE_HOST_WIN32)
        MESSAGE(FATAL_ERROR, "Undefined VCPKG_ROOT in Windows, CMake will end.")
    endif ()
endif ()

PROJECT(httpdns_c_sdk LANGUAGES C)

####################################  编译选项  ###########################################
# default Release C flags
if (CMAKE_HOST_LINUX OR CMAKE_HOST_APPLE)
    set(CMAKE_C_FLAGS " -g -ggdb -O0 -w -fpic -fPIC -D_LARGEFILE64_SOURCE")
    set(CMAKE_CXX_FLAGS " -g -ggdb -O0 -w -fpic -fPIC -D_LARGEFILE64_SOURCE")

    set(CMAKE_C_FLAGS_DEBUG " -g -ggdb -O0 -w -fpic -fPIC -D_LARGEFILE64_SOURCE")
    set(CMAKE_CXX_FLAGS_DEBUG " -g -ggdb -O0 -w -fpic -fPIC -D_LARGEFILE64_SOURCE")

    set(CMAKE_C_FLAGS_RELEASE " -O3 -w -fpic -fPIC -D_LARGEFILE64_SOURCE")
    set(CMAKE_CXX_FLAGS_RELEASE " -O3 -w -fpic -fPIC -D_LARGEFILE64_SOURCE")

    set(CMAKE_C_FLAGS_MINSIZEREF " -Os -w  -fpic -fPIC -D_LARGEFILE64_SOURCE")
    set(CMAKE_CXX_FLAGS_MINSIZEREF " -Os -w -fpic -fPIC -D_LARGEFILE64_SOURCE")

    set(CMAKE_C_FLAGS_RELWITHDEBINFO " -O2 -w -fpic -fPIC -D_LARGEFILE64_SOURCE")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO " -O2 -w -fpic -fPIC -D_LARGEFILE64_SOURCE")

    set(CMAKE_C_FLAGS_COVERAGE " ${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS_COVERAGE " ${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")

    set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}/lib)
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}/bin)
    set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
elseif (CMAKE_HOST_WIN32)
    set(CMAKE_C_FLAGS " /O2 /W0")
    set(CMAKE_CXX_FLAGS " /O2 /W0")

    set(CMAKE_C_FLAGS_DEBUG " /Zi /Od /W0")
    set(CMAKE_CXX_FLAGS_DEBUG " /Zi /Od /W0")

    set(CMAKE_C_FLAGS_RELEASE " /O2 /W0")
    set(CMAKE_CXX_FLAGS_RELEASE " /O2 /W0")

    set(CMAKE_C_FLAGS_MINSIZEREF " /O1 /W0")
    set(CMAKE_CXX_FLAGS_MINSIZEREF " /O1 /W0")

    set(CMAKE_C_FLAGS_RELWITHDEBINFO " /O2 /Zi /W0")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO " /O2 /Zi /W0")
else ()
    MESSAGE(FATAL_ERROR, "Unsupported OS for setting CMAKE_C_FLAGS, CMake will end.")
endif ()

option(ADD_LOG_USE_COLOR "ON for enabling color printing" ON)
IF (ADD_LOG_USE_COLOR)
    add_definitions(-DLOG_USE_COLOR)
ENDIF (ADD_LOG_USE_COLOR)

option(ADD_TEST_DEBUG_LOG "ON for unit test logs" ON)
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

################################# 获取依赖库 ############################################
if (DEFINED VCPKG_ROOT)
    find_package(cJSON REQUIRED)
    find_package(OpenSSL REQUIRED)
    find_package(CURL REQUIRED)
    find_path(APR_INCLUDE_DIR NAMES apr.h)
    find_path(APU_INCLUDE_DIR NAMES apu.h)
    if (CMAKE_HOST_WIN32)
        find_library(APU_LIBRARIES libaprutil-1)
        find_library(APR_LIBRARIES libapr-1)
    else ()
        find_library(APU_LIBRARIES aprutil-1)
        find_library(APR_LIBRARIES apr-1)
    endif ()
else ()
    find_package(cJSON REQUIRED)
    find_package(OpenSSL REQUIRED)
    find_package(CURL REQUIRED)
    if (CMAKE_HOST_LINUX)
        FIND_PROGRAM(APR_CONFIG_BIN NAMES apr-config apr-1-config PATHS /usr/bin /usr/local/bin /usr/local/apr/bin/)
        FIND_PROGRAM(APU_CONFIG_BIN NAMES apu-config apu-1-config PATHS /usr/bin /usr/local/bin /usr/local/apr/bin/)
    elseif (CMAKE_HOST_APPLE)
        EXECUTE_PROCESS(
                COMMAND brew --prefix apr
                OUTPUT_VARIABLE APR_PATH
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        SET(APR_CONFIG_BIN "${APR_PATH}/bin/apr-1-config")
        EXECUTE_PROCESS(
                COMMAND brew --prefix apr-util
                OUTPUT_VARIABLE APU_PATH
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        SET(APU_CONFIG_BIN "${APU_PATH}/bin/apu-1-config")
    else ()
        MESSAGE(FATAL_ERROR, "Unsupported OS for getting apr/apu path, CMake will end.")
    endif ()

    IF (APR_CONFIG_BIN)
        EXECUTE_PROCESS(
                COMMAND ${APR_CONFIG_BIN} --includedir
                OUTPUT_VARIABLE APR_INCLUDE_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(STRIP ${APR_INCLUDE_DIR} APR_INCLUDE_DIR)
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
                OUTPUT_VARIABLE APU_INCLUDE_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(STRIP ${APU_INCLUDE_DIR} APU_INCLUDE_DIR)
        EXECUTE_PROCESS(
                COMMAND ${APU_CONFIG_BIN} --link-ld
                OUTPUT_VARIABLE APU_LIBRARIES
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(STRIP ${APU_LIBRARIES} APU_LIBRARIES)
    ELSE ()
        MESSAGE(FATAL_ERROR "Could not find apu-config/apu-1-config")
    ENDIF ()
endif ()
MESSAGE(STATUS "\n")
MESSAGE(STATUS "CURL_INCLUDE_DIRS:${CURL_INCLUDE_DIRS}")
MESSAGE(STATUS "APU_INCLUDE_DIR:${APU_INCLUDE_DIR}")
MESSAGE(STATUS "APR_INCLUDE_DIR:${APR_INCLUDE_DIR}")
MESSAGE(STATUS "CJSON_INCLUDE_DIR:${CJSON_INCLUDE_DIR}")
MESSAGE(STATUS "OPENSSL_INCLUDE_DIR:${OPENSSL_INCLUDE_DIR}")
MESSAGE(STATUS "\n")
MESSAGE(STATUS "CURL_LIBRARIES:${CURL_LIBRARIES}")
MESSAGE(STATUS "APU_LIBRARIES:${APU_LIBRARIES}")
MESSAGE(STATUS "APR_LIBRARIES:${APR_LIBRARIES}")
MESSAGE(STATUS "CJSON_LIBRARIES:${CJSON_LIBRARIES}")
MESSAGE(STATUS "OPENSSL_LIBRARIES:${OPENSSL_LIBRARIES}")
MESSAGE(STATUS "\n")
################################## 库文件编译安装 ############################################
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${APU_INCLUDE_DIR})
include_directories(${APR_INCLUDE_DIR})
include_directories(${CJSON_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})

aux_source_directory(src SRC_LIST)
## 静态库安装
add_library(${CMAKE_PROJECT_NAME}_static STATIC ${SRC_LIST})
target_link_libraries(${CMAKE_PROJECT_NAME}_static PRIVATE ${OPENSSL_LIBRARIES} ${CURL_LIBRARIES} ${CJSON_LIBRARIES} ${APU_LIBRARIES} ${APR_LIBRARIES})
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(${CMAKE_PROJECT_NAME}_static PRIVATE dl)
endif ()
INSTALL(TARGETS ${CMAKE_PROJECT_NAME}_static ARCHIVE DESTINATION lib)

## 动态库安装
add_library(${CMAKE_PROJECT_NAME} SHARED ${SRC_LIST})
target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC BUILD_SHARED_LIB)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES VERSION ${CMAKE_VERSION} SOVERSION ${CMAKE_VERSION})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${OPENSSL_LIBRARIES} ${CURL_LIBRARIES} ${CJSON_LIBRARIES} ${APU_LIBRARIES} ${APR_LIBRARIES})
if (CMAKE_HOST_LINUX)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE dl)
endif ()
INSTALL(TARGETS ${CMAKE_PROJECT_NAME} LIBRARY DESTINATION lib)


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
add_subdirectory(tests)
add_subdirectory(examples)