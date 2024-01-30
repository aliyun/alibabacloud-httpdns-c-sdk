# Install script for directory: /Users/cgs/Documents/project/alicloud-httpdns-sdk-c

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Library/Developer/CommandLineTools/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/cmake-build-release/build/Release/lib/libhttpdns_c_sdk_static.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libhttpdns_c_sdk_static.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libhttpdns_c_sdk_static.a")
    execute_process(COMMAND "/Library/Developer/CommandLineTools/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libhttpdns_c_sdk_static.a")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/httpdns_c_sdk" TYPE FILE FILES
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/cJSON.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/cJSON_Utils.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/configuration.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/dict.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/http_response_parser.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_cache.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_client.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_client_config.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_error_type.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_global_config.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_http.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_ip.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_list.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_memory.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_multi_thread.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_resolve_request.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_resolve_result.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_resolver.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_scheduler.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_sign.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/httpdns_time.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/list.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/log.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/net_stack_detector.h"
    "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/src/sds.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/cmake-build-release/tests/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/Users/cgs/Documents/project/alicloud-httpdns-sdk-c/cmake-build-release/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
