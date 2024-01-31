//
// Created by cagaoshuai on 2024/1/31.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_CLIENT_WRAPPER_H
#define HTTPDNS_C_SDK_HTTPDNS_CLIENT_WRAPPER_H

#include  "httpdns_resolve_result.h"
#include "httpdns_client.h"

#define MULTI_RESOLVE_SIZE 5

/**
 * 初始化htptdns client 环境
 * @param account_id   HTTPDNS账户ip
 * @param secret_key   HTTPDNS加签秘钥，如果设置为空则采用非加签访问
 * @return
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
httpdns_config_t *get_httpdns_client_config();

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
 *  @return 解析结果
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
 *  @return 解析结果
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
 *      httpdns_list_free(results, DATA_FREE_FUNC(httpdns_resolve_result_free))进行释放，否则会造成内存泄露
 */
int32_t
batch_get_httpdns_result_for_hosts_sync_with_cache(struct list_head *hosts,
                                                   const char *query_type,
                                                   const char *client_ip,
                                                   struct list_head *results);
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
 *      解析结果使用完毕后，需要调用httpdns_list_free(results, DATA_FREE_FUNC(httpdns_resolve_result_free))进行释放，否则会造成内存泄露
 */
int32_t batch_get_httpdns_result_for_hosts_sync_without_cache(struct list_head *hosts,
                                                              const char *query_type,
                                                              struct list_head *results,
                                                              const char *client_ip);

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

#endif //HTTPDNS_C_SDK_HTTPDNS_CLIENT_WRAPPER_H
