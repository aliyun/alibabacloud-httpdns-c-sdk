PROJECT(alicloud_httpdns_c_sdk_examples LANGUAGES C CXX)

##################################### Example Application ################################
INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${APU_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${APR_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${CJSON_INCLUDE_DIR})
INCLUDE_DIRECTORIES(../src)

FUNCTION(_TARGET_EXAMPLE_LIBRARIES EXAMPLE_BIN_NAME SOURCE_FILES)
    ADD_EXECUTABLE(${EXAMPLE_BIN_NAME} ${SOURCE_FILES})
    TARGET_LINK_LIBRARIES(${EXAMPLE_BIN_NAME} PRIVATE ${CMAKE_PROJECT_NAME}_static ${CURL_LIBRARIES} ${CJSON_LIBRARIES} ${APU_LIBRARIES} ${APR_LIBRARIES})
    IF (CMAKE_HOST_UNIX AND NOT CMAKE_HOST_APPLE)
        TARGET_LINK_LIBRARIES(${EXAMPLE_BIN_NAME} PRIVATE dl)
    ENDIF ()
ENDFUNCTION()

_TARGET_EXAMPLE_LIBRARIES(sync_resolve_example sync_resolve_example.c)

_TARGET_EXAMPLE_LIBRARIES(cxx_integration_example cxx_integration_example.cpp)

_TARGET_EXAMPLE_LIBRARIES(async_resolve_example async_resolve_example.c)

_TARGET_EXAMPLE_LIBRARIES(sdns_resolve_example sdns_resolve_example.c)

_TARGET_EXAMPLE_LIBRARIES(batch_resolve_example batch_resolve_example.c)

##################################### OSS Adapter Example ################################
# 注意：此示例需要额外链接 aliyun-oss-cpp-sdk
# 编译前请确保已安装 OSS SDK，并设置 OSS_SDK_INCLUDE_DIR 和 OSS_SDK_LIBRARIES
#
# 示例编译命令：
# cmake .. -DOSS_SDK_INCLUDE_DIR=/path/to/oss-sdk/include \
#          -DOSS_SDK_LIBRARIES=/path/to/libalibabacloud-oss-cpp-sdk.a
#
IF(OSS_SDK_INCLUDE_DIR AND OSS_SDK_LIBRARIES)
    MESSAGE(STATUS "OSS SDK found, building oss_httpdns_example")
    INCLUDE_DIRECTORIES(${OSS_SDK_INCLUDE_DIR})
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/oss_adapter)
    
    ADD_EXECUTABLE(oss_httpdns_example oss_adapter/oss_httpdns_example.cpp)
    TARGET_LINK_LIBRARIES(oss_httpdns_example PRIVATE 
        ${CMAKE_PROJECT_NAME}_static 
        ${OSS_SDK_LIBRARIES}
        ${CURL_LIBRARIES} 
        ${CJSON_LIBRARIES} 
        ${APU_LIBRARIES} 
        ${APR_LIBRARIES})
    
    # OpenSSL (OSS SDK 依赖)
    FIND_PACKAGE(OpenSSL)
    IF(OPENSSL_FOUND)
        TARGET_LINK_LIBRARIES(oss_httpdns_example PRIVATE ${OPENSSL_LIBRARIES})
    ENDIF()
    
    IF(CMAKE_HOST_UNIX AND NOT CMAKE_HOST_APPLE)
        TARGET_LINK_LIBRARIES(oss_httpdns_example PRIVATE dl pthread)
    ENDIF()
ENDIF()