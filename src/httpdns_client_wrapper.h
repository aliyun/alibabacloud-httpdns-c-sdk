//
// 面向上层应用的接口封装，提升SDK易用性
//
// Created by caogaoshuai on 2024/1/31.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_CLIENT_WRAPPER_H
#define HTTPDNS_C_SDK_HTTPDNS_CLIENT_WRAPPER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "httpdns_client_config.h"
#include "httpdns_resolve_request.h"
#include  "httpdns_resolve_result.h"


#define HTTPDNS_MULTI_RESOLVE_SIZE 5

/**
 * 初始化htptdns client 环境
 * @param account_id   HTTPDNS账户ip
 * @param secret_key   HTTPDNS加签秘钥，如果设置为空则采用非加签访问
 * @note must free using httpdns_client_env_cleanup
 *
 */
int32_t httpdns_client_env_init(const char *account_id,
                                const char *secret_key);

/**
 * 清理httpdns client 环境
 */
int32_t httpdns_client_env_cleanup();

/**
 * 获取当前所使用的客户端配置，用于正式解析之前自定义配置
 */
httpdns_config_t *httpdns_client_get_config();

/**
 * 预加载域名解析
 */
void httpdns_client_process_pre_resolve_hosts();

/**
 *
 * 单域名同步解析，阻塞线程，直到结果返回或者超时
 *
 *  @param request 自定义的httpdns 请求
 *  @return 解析结果，如果解析失败则返回NULL
 *  @note
 *      解析结果使用完毕后，需要调用httpdns_resolve_result_free进行释放，否则会造成内存泄露
 */
httpdns_resolve_result_t *get_httpdns_result_for_host_sync_with_custom_request(httpdns_resolve_request_t *request);

/**
 *
 * 单域名同步解析，阻塞线程，先查缓存，缓存为空则查询HTTPDNS服务器，直到结果返回或者超时
 *
 * @param host 待解析的域名
 * @param query_type 解析类型，可选一下四种宏
 *  - HTTPDNS_QUERY_TYPE_AUTO       根据网络类型自动解析对应类型的记录
 *  - HTTPDNS_QUERY_TYPE_A          解析域名的A记录
 *  - HTTPDNS_QUERY_TYPE_AAAA       解析域名的AAAA记录
 *  - HTTPDNS_QUERY_TYPE_BOTH       同时解析域名的A和AAAA记录
 *  @param client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 *  @return 解析结果，如果解析失败则返回NULL
 *  @note
 *      解析结果使用完毕后，需要调用httpdns_resolve_result_free进行释放，否则会造成内存泄露
 */
httpdns_resolve_result_t *get_httpdns_result_for_host_sync_with_cache(const char *host,
                                                                      const char *query_type,
                                                                      const char *client_ip);

/**
 *
 * 同步解析，阻塞线程，查询HTTPDNS服务器，直到结果返回或者超时
 *
 * @param host 待解析的域名
 * @param query_type 解析类型，可选一下四种宏
 *  - HTTPDNS_QUERY_TYPE_AUTO       根据网络类型自动解析对应类型的记录
 *  - HTTPDNS_QUERY_TYPE_A          解析域名的A记录
 *  - HTTPDNS_QUERY_TYPE_AAAA       解析域名的AAAA记录
 *  - HTTPDNS_QUERY_TYPE_BOTH       同时解析域名的A和AAAA记录
 * @param client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 *  @return 解析结果，如果解析失败则返回NULL
 *  @note
 *      解析结果使用完毕后，需要调用httpdns_resolve_result_free进行释放，否则会造成内存泄露
 */
httpdns_resolve_result_t *get_httpdns_result_for_host_sync_without_cache(const char *host,
                                                                         const char *query_type,
                                                                         const char *client_ip);

/**
 *
 * 批量同步解析，阻塞线程，先查缓存，缓存为空则查询HTTPDNS服务器，直到结果返回或者超时
 *
 * @param hosts 待解析的域名列表
 * @param query_type 解析类型，可选一下四种宏
 *  - HTTPDNS_QUERY_TYPE_AUTO       根据网络类型自动解析对应类型的记录
 *  - HTTPDNS_QUERY_TYPE_A          解析域名的A记录
 *  - HTTPDNS_QUERY_TYPE_AAAA       解析域名的AAAA记录
 *  - HTTPDNS_QUERY_TYPE_BOTH       同时解析域名的A和AAAA记录
 *  @param client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 *  @param results 存储解析结果的链表指针
 *  @return 函数调用结果，成功时为HTTPDNS_SUCCESS
 *  @note
 *      解析结果使用完毕后，需要调用
 *      httpdns_list_free(results, to_httpdns_data_free_func(httpdns_resolve_result_free))进行释放，否则会造成内存泄露
 */
int32_t
get_httpdns_results_for_hosts_sync_with_cache(httpdns_list_head_t *hosts,
                                              const char *query_type,
                                              const char *client_ip,
                                              httpdns_list_head_t *results);

/**
 *
 * 批量同步解析，阻塞线程，查询HTTPDNS服务器，直到结果返回或者超时
 *
 * @param host 待解析的域名
 * @param query_type 解析类型，可选一下四种宏
 *  - HTTPDNS_QUERY_TYPE_AUTO       根据网络类型自动解析对应类型的记录
 *  - HTTPDNS_QUERY_TYPE_A          解析域名的A记录
 *  - HTTPDNS_QUERY_TYPE_AAAA       解析域名的AAAA记录
 *  - HTTPDNS_QUERY_TYPE_BOTH       同时解析域名的A和AAAA记录
 *  @param client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 *  @param results 存储解析结果的链表指针
 *  @return 函数调用结果，成功时为HTTPDNS_SUCCESS
 *  @note
 *      解析结果使用完毕后，需要调用httpdns_list_free(results, to_httpdns_data_free_func(httpdns_resolve_result_free))进行释放，否则会造成内存泄露
 */
int32_t get_httpdns_results_for_hosts_sync_without_cache(httpdns_list_head_t *hosts,
                                                         const char *query_type,
                                                         httpdns_list_head_t *results,
                                                         const char *client_ip);

/**
 *
 * 单域名异步解析，不阻塞线程，直到结果返回或者超时
 *
 *  @param request 自定义的httpdns 请求
 *  @return 解析结果，如果解析失败则返回NULL
 *  @note
 *      解析结果使用完毕后，需要调用httpdns_resolve_result_free进行释放，否则会造成内存泄露
 */
int32_t get_httpdns_result_for_host_async_with_custom_request(httpdns_resolve_request_t *request);


/**
 *
 * 异步解析，不阻塞线程，先查缓存，缓存为空则查询服务器，解析完成时调用回调函数
 *
 * @param host 待解析的域名
 * @param query_type 解析类型，可选一下四种宏
 *  - HTTPDNS_QUERY_TYPE_AUTO       根据网络类型自动解析对应类型的记录
 *  - HTTPDNS_QUERY_TYPE_A          解析域名的A记录
 *  - HTTPDNS_QUERY_TYPE_AAAA       解析域名的AAAA记录
 *  - HTTPDNS_QUERY_TYPE_BOTH       同时解析域名的A和AAAA记录
 *
 *  @param client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 */
int32_t get_httpdns_result_for_host_async_with_cache(const char *host,
                                                     const char *query_type,
                                                     const char *client_ip,
                                                     httpdns_complete_callback_func_t cb,
                                                     void *cb_param);

/**
 *
 * 异步解析，不阻塞线程，查询服务器，解析完成时调用回调函数
 *
 * @param host 待解析的域名
 * @param query_type 解析类型，可选一下四种宏
 *  - HTTPDNS_QUERY_TYPE_AUTO       根据网络类型自动解析对应类型的记录
 *  - HTTPDNS_QUERY_TYPE_A          解析域名的A记录
 *  - HTTPDNS_QUERY_TYPE_AAAA       解析域名的AAAA记录
 *  - HTTPDNS_QUERY_TYPE_BOTH       同时解析域名的A和AAAA记录
 *
 *  @param client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 */
int32_t get_httpdns_result_for_host_async_without_cache(const char *host,
                                                        const char *query_type,
                                                        const char *client_ip,
                                                        httpdns_complete_callback_func_t cb,
                                                        void *cb_param);


/**
 *
 * 批量异步解析，不阻塞线程，先查缓存，缓存为空则查询HTTPDNS服务器，直到结果返回或者超时
 *
 * @param hosts 待解析的域名列表
 * @param query_type 解析类型，可选一下四种宏
 *  - HTTPDNS_QUERY_TYPE_AUTO       根据网络类型自动解析对应类型的记录
 *  - HTTPDNS_QUERY_TYPE_A          解析域名的A记录
 *  - HTTPDNS_QUERY_TYPE_AAAA       解析域名的AAAA记录
 *  - HTTPDNS_QUERY_TYPE_BOTH       同时解析域名的A和AAAA记录
 *  @param client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 *  @return 函数调用结果，成功时为HTTPDNS_SUCCESS
 *  @note
 *      结果通过回调函数透出
 */
int32_t
get_httpdns_results_for_hosts_async_with_cache(httpdns_list_head_t *hosts,
                                               const char *query_type,
                                               const char *client_ip,
                                               httpdns_complete_callback_func_t cb,
                                               void *cb_param);

/**
 *
 * 批量异步解析，不阻塞调用线程，直接查询HTTPDNS服务器，直到结果返回或者超时
 *
 * @param host 待解析的域名
 * @param query_type 解析类型，可选一下四种宏
 *  - HTTPDNS_QUERY_TYPE_AUTO       根据网络类型自动解析对应类型的记录
 *  - HTTPDNS_QUERY_TYPE_A          解析域名的A记录
 *  - HTTPDNS_QUERY_TYPE_AAAA       解析域名的AAAA记录
 *  - HTTPDNS_QUERY_TYPE_BOTH       同时解析域名的A和AAAA记录
 *  @param client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 *  @return 函数调用结果，成功时为HTTPDNS_SUCCESS
 *  @note
 *      结果通过回调函数透出
 */
int32_t get_httpdns_results_for_hosts_async_without_cache(httpdns_list_head_t *hosts,
                                                          const char *query_type,
                                                          const char *client_ip,
                                                          httpdns_complete_callback_func_t cb,
                                                          void *cb_param);

int32_t select_ip_from_httpdns_result(httpdns_resolve_result_t *result, char *dst_ip_buffer);

#ifdef __cplusplus
}
#endif

#endif //HTTPDNS_C_SDK_HTTPDNS_CLIENT_WRAPPER_H
