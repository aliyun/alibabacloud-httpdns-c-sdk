//
// Created by cagaoshuai on 2024/4/9.
//

#ifndef HDNS_C_SDK_HDNS_API_H
#define HDNS_C_SDK_HDNS_API_H

#include "hdns_config.h"
#include "hdns_status.h"
#include "hdns_resolver.h"
#include "hdns_client.h"
#include "hdns_log.h"

HDNS_CPP_START

/*
 * @brief 异步解析回调函数
 * @param[in]   status    解析最终状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @param[in]   results   解析结果列表
 * @param[in]   param     用户传递的自定义参数
 */
typedef void (*hdns_resv_done_callback_pt)(hdns_status_t *status, hdns_list_head_t *results, void *param);

/*
 * @brief SDK环境初始化，主要包括全局随机数、session池、网络检测器、线程池
 * @return 0：初始化成功；1：初始化失败
 * @note：
 *  - 调用该接口之后，请务必调用一次hdns_sdk_cleanup进行资源释放
 *  - 该接口并非线程安全接口
 */
int hdns_sdk_init();

/*
 * @brief 创建客户端实例
 * @param[in]   account_id    HTTPDNS账户ID
 * @param[in]   secret_key    用于加签HTTP请求的秘钥，如果不需要加签可以填NULL
 * @return 创建成功返回客户端实例，否则返回NULL
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
hdns_client_t *hdns_client_create(const char *account_id, const char *secret_key);

/*
 * @brief  启动客户端，执行域名的异步预解析和拉取域名解析服务器列表
 * @param[in]   client        客户端实例
 * @return  客户端启动状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
hdns_status_t hdns_client_start(hdns_client_t *client);


/*
 * @brief  设置本地缓存失效时，客户端单次请求服务端的最大超时时间
 * @param[in]   client        客户端实例
 * @param[in]   timeout       超时毫秒数
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
void hdns_client_set_timeout(hdns_client_t *client, int32_t timeout);

/*
 * @brief  设置服务是否使用本地缓存
 * @param[in]   client        客户端实例
 * @param[in]   using_cache    false:不使用本地缓存，true:使用本地缓存
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
void hdns_client_set_using_cache(hdns_client_t *client, bool using_cache);

/*
 * @brief  设置访问HTTPDNS服务器时是否使用https协议
 * @param[in]   client        客户端实例
 * @param[in]   using_https    false:不使用https协议，true:使用https协议
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
void hdns_client_set_using_https(hdns_client_t *client, bool using_https);


/*
 * @brief  设置访问HTTPDNS服务器时是否对请求进行签名
 * @param[in]   client        客户端实例
 * @param[in]   using_sign    false:签名，true:签名
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
void hdns_client_set_using_sign(hdns_client_t *client, bool using_sign);

/*
 * @brief   设置访问HTTPDNS服务器时的重试次数，默认为1
 * @param[in]   client        客户端实例
 * @param[in]   retry_times   重试次数
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
void hdns_client_set_retry_times(hdns_client_t *client, int32_t retry_times);

/*
 * @brief   设置访问HTTPDNS服务器的集群
 * @param[in]   client        客户端实例
 * @param[in]   region        global: 就近访问（默认），cn:中国大陆，hk:中国香港，sg: 新加坡，us: 美国，de: 德国
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
void hdns_client_set_region(hdns_client_t *client, const char *region);

/*
 * @brief   设置HTTPDNS调度中心的region
 * @param[in]   client        客户端实例
 * @param[in]   region        cn:中国大陆（默认），hk:中国香港，sg: 新加坡，us: 美国，de: 德国
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
void hdns_client_set_schedule_center_region(hdns_client_t *client, const char *region);

/*
 * @brief   是否开启网络变化时缓存更新
 * @param[in]   client        客户端实例
 * @param[in]   enable        true: 刷新，false：不刷新
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
void hdns_client_enable_update_cache_after_net_change(hdns_client_t *client, bool enable);

/*
 * @brief   是否开启允许使用过期ip
 * @param[in]   client        客户端实例
 * @param[in]   enable        true: 允许使用过期ip，false：不允许
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
void hdns_client_enable_expired_ip(hdns_client_t *client, bool enable);

/*
 * @brief   是否允许降级到localdns
 * @param[in]   client        客户端实例
 * @param[in]   enable        true: 允许使用过期ip，false：不允许
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
void hdns_client_enable_failover_localdns(hdns_client_t *client, bool enable);

/*
 * @brief   添加客户端启动时预解析的域名
 * @param[in]   client        客户端实例
 * @param[in]   host          预解析域名
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
void hdns_client_add_pre_resolve_host(hdns_client_t *client, const char *host);

/*
 * @brief   添加进行IP嗅探的域名和端口，一个域名只允许探测一个端口
 * @param[in]   client        客户端实例
 * @param[in]   host          探测域名
 * @param[in]   port          探测端口
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
void hdns_client_add_ip_probe_item(hdns_client_t *client, const char *host, const int port);


/*
 * @brief   针对某个域名，添加一个自定义的解析ttl，仅对HTTPDNS的解析结果有效，降级到localdns无效
 * @param[in]   client        客户端实例
 * @param[in]   host          探测域名
 * @param[in]   ttl           自定义ttl
 * @note :
 *    - hdns_client_t是线程安全的，可多线程共享
 */
void hdns_client_add_custom_ttl_item(hdns_client_t *client, const char *host, const int ttl);


/*
 * @brief   获取客户端的session id，用于问题排查
 * @param[in]   client        客户端实例
 * @param[out]   session_id    session id
 * @return 0 获取成功，否则失败
 * @note:
 *    - session id 为长度12的字符串，请确保接收的buffer大于12
 */
int hdns_client_get_session_id(hdns_client_t *client, char *session_id);

/*
 * @brief   自定义域名解析时，创建解析请求实例
 * @param[in]   client        客户端实例
 * @return  该客户端对应的请求实例
 * @note :
 *    - hdns_resv_req_t是线程安全的，可多线程共享
 */
hdns_resv_req_t *hdns_resv_req_create(hdns_client_t *client);

/*
 * @brief   自定义域名解析时，设置请求实例的客户端IP
 * @param[in]   client        客户端实例
 * @param[in]   client_ip     客户端IP
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_resv_req_t是线程安全的，可多线程共享
 */
hdns_status_t hdns_resv_req_set_client_ip(hdns_resv_req_t *req, const char *client_ip);

/*
 * @brief   自定义域名解析时，设置请求实例的域名
 * @param[in]   req          请求实例
 * @param[in]   host          待解析域名
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_resv_req_t是线程安全的，可多线程共享
 */
hdns_status_t hdns_resv_req_set_host(hdns_resv_req_t *req, const char *host);

/*
 * @brief   自定义域名解析时，添加业务参数对
 * @param[in]   req          请求实例
 * @param[in]   key           参数名
 * @param[in]   value         参数值
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_resv_req_t是线程安全的，可多线程共享
 */
hdns_status_t hdns_resv_req_append_sdns_param(hdns_resv_req_t *req, const char *key, const char *value);

/*
 * @brief   自定义域名解析时，设置请求实例的DNS解析类型
 * @param[in]   req          请求实例
 * @param[in]   query_type    请求类型
 *         - HDNS_QUERY_AUTO：根据网络栈自动解析；
 *         - HDNS_QUERY_IPV4：解析IPV4类型；
 *         - HDNS_QUERY_IPV6：解析IPv6类型
 *         - HDNS_QUERY_BOTH：解析IPV4和IPV6类型
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_resv_req_t是线程安全的，可多线程共享
 */
hdns_status_t hdns_resv_req_set_query_type(hdns_resv_req_t *req, hdns_query_type_t query_type);

/*
 * @brief   自定义域名解析时，设置解析结果的缓存key
 * @param[in]   req          请求实例
 * @param[in]   key           参数名
 * @param[in]   value         参数值
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_resv_req_t是线程安全的，可多线程共享
 */
hdns_status_t hdns_resv_req_set_cache_key(hdns_resv_req_t *req, const char *cache_key);

/*
 * @brief   自定义域名解析时，释放请求实例资源
 * @param[in]   req          请求实例
 */
void hdns_resv_req_cleanup(hdns_resv_req_t *req);

/*
 * @brief   创建链表实例
 * @return  链表实例，如果失败返回NULL
 * @note :
 *    - hdns_list_head_t非线程安全，多线程共享时应该通过互斥量等手段进行同步
 */
hdns_list_head_t *hdns_list_create();

/*
 * @brief   向链表中添加字符串
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_list_head_t非线程安全，多线程共享时应该通过互斥量等手段进行同步
 */
hdns_status_t hdns_list_add_str(hdns_list_head_t *list, const char *str);

/*
 * @brief   释放链表资源
 * @param[in]   list          待释放的列表实例
 * @note :
 *    - hdns_list_head_t非线程安全，多线程共享时应该通过互斥量等手段进行同步
 */
void hdns_list_cleanup(hdns_list_head_t *list);

/*
 * @brief   自定义同步解析
 * @param[in]   client          客户端实例
 * @param[in]   req             自定义解析请求实例
 * @param[out]  results         解析结果，需要通过hdns_list_cleanup进行内存释放
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_list_head_t是非线程安全的，多线程共享时应该通过互斥量等手段进行同步
 */
hdns_status_t hdns_get_result_for_host_sync_with_custom_request(hdns_client_t *client,
                                                                const hdns_resv_req_t *req,
                                                                hdns_list_head_t **results);


/*
 *
 * @brief 单域名同步解析，阻塞线程，先查缓存，缓存为空则查询HTTPDNS服务器，直到结果返回或者超时
 * @param[in]   client          客户端实例
 * @param[in]      host          待解析的域名
 * @param[in]      query_type    请求类型
 *         - HDNS_QUERY_AUTO：根据网络栈自动解析；
 *         - HDNS_QUERY_IPV4：解析IPV4类型；
 *         - HDNS_QUERY_IPV6：解析IPv6类型
 *         - HDNS_QUERY_BOTH：解析IPV4和IPV6类型
 * @param[in]      client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 * @param[out]     results         解析结果，需要通过hdns_list_cleanup进行内存释放
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_client_t线程安全，可多线程共享
 */
hdns_status_t hdns_get_result_for_host_sync_with_cache(hdns_client_t *client,
                                                       const char *host,
                                                       hdns_query_type_t query_type,
                                                       const char *client_ip,
                                                       hdns_list_head_t **results);


/*
 *
 * @brief 单域名同步解析，阻塞线程，查询HTTPDNS服务器，直到结果返回或者超时
 * @param[in]      client        客户端实例
 * @param[in]      host          待解析的域名
 * @param[in]      query_type    请求类型
 *         - HDNS_QUERY_AUTO：根据网络栈自动解析；
 *         - HDNS_QUERY_IPV4：解析IPV4类型；
 *         - HDNS_QUERY_IPV6：解析IPv6类型
 *         - HDNS_QUERY_BOTH：解析IPV4和IPV6类型
 * @param[in]      client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 * @param[out]     results         解析结果，需要通过hdns_list_cleanup进行内存释放
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_client_t线程安全，可多线程共享
 */
hdns_status_t hdns_get_result_for_host_sync_without_cache(hdns_client_t *client,
                                                          const char *host,
                                                          hdns_query_type_t query_type,
                                                          const char *client_ip,
                                                          hdns_list_head_t **results);

/*
 *
 * @brief 批量域名同步解析，阻塞线程，先查缓存，缓存为空则查询HTTPDNS服务器，直到结果返回或者超时
 *
 * @param[in]      client          客户端实例
 * @param[in]      hosts         待解析的域名列表
 * @param[in]      query_type    请求类型
 *         - HDNS_QUERY_AUTO：根据网络栈自动解析；
 *         - HDNS_QUERY_IPV4：解析IPV4类型；
 *         - HDNS_QUERY_IPV6：解析IPv6类型
 *         - HDNS_QUERY_BOTH：解析IPV4和IPV6类型
 * @param[in]      client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 * @param[out]     results         解析结果，需要通过hdns_list_cleanup进行内存释放
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_client_t线程安全，可多线程共享
 */
hdns_status_t hdns_get_results_for_hosts_sync_with_cache(hdns_client_t *client,
                                                         const hdns_list_head_t *hosts,
                                                         hdns_query_type_t query_type,
                                                         const char *client_ip,
                                                         hdns_list_head_t **results);

/*
 *
 * @brief 批量域名同步解析，阻塞线程，查询HTTPDNS服务器，直到结果返回或者超时
 *
 * @param[in]      client        客户端实例
 * @param[in]      hosts         待解析的域名列表
 * @param[in]      query_type    请求类型
 *         - HDNS_QUERY_AUTO：根据网络栈自动解析；
 *         - HDNS_QUERY_IPV4：解析IPV4类型；
 *         - HDNS_QUERY_IPV6：解析IPv6类型
 *         - HDNS_QUERY_BOTH：解析IPV4和IPV6类型
 * @param[in]      client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 * @param[out]     results         解析结果，需要通过hdns_list_cleanup进行内存释放
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_client_t线程安全，可多线程共享
 */
hdns_status_t hdns_get_results_for_hosts_sync_without_cache(hdns_client_t *client,
                                                            const hdns_list_head_t *hosts,
                                                            hdns_query_type_t query_type,
                                                            const char *client_ip,
                                                            hdns_list_head_t **results);

/*
 * @brief   先进行自定义异步步解析，最后触发函数回调
 * @param[in]   client          客户端实例
 * @param[in]   req             自定义解析请求实例
 * @param[in]   cb              解析结束后的回调函数
 * @param[in]   cb_param        回调函数的用户自定义参数
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_client_t线程安全，可多线程共享
 */
hdns_status_t hdns_get_result_for_host_async_with_custom_request(hdns_client_t *client,
                                                                 const hdns_resv_req_t *resv_req,
                                                                 hdns_resv_done_callback_pt cb,
                                                                 void *cb_param);

/*
 *
 * @brief 单域名异步解析，不阻塞线程，先查缓存，缓存为空则查询HTTPDNS服务器，直到结果返回或者超时，最后触发函数回调
 *
 * @param[in]      client        客户端实例
 * @param[in]      host          待解析的域名
 * @param[in]      query_type    请求类型
 *         - HDNS_QUERY_AUTO：根据网络栈自动解析；
 *         - HDNS_QUERY_IPV4：解析IPV4类型；
 *         - HDNS_QUERY_IPV6：解析IPv6类型
 *         - HDNS_QUERY_BOTH：解析IPV4和IPV6类型
 * @param[in]      client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 * @param[in]   cb              解析结束后的回调函数
 * @param[in]   cb_param        回调函数的用户自定义参数
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_client_t线程安全，可多线程共享
 */
hdns_status_t hdns_get_result_for_host_async_with_cache(hdns_client_t *client,
                                                        const char *host,
                                                        hdns_query_type_t query_type,
                                                        const char *client_ip,
                                                        hdns_resv_done_callback_pt cb,
                                                        void *cb_param);

/*
 *
 * @brief 单域名异步解析，不阻塞线程，查询HTTPDNS服务器，直到结果返回或者超时，，最后触发函数回调
 *
 * @param[in]      client        客户端实例
 * @param[in]      host          待解析的域名
 * @param[in]      query_type    请求类型
 *         - HDNS_QUERY_AUTO：根据网络栈自动解析；
 *         - HDNS_QUERY_IPV4：解析IPV4类型；
 *         - HDNS_QUERY_IPV6：解析IPv6类型
 *         - HDNS_QUERY_BOTH：解析IPV4和IPV6类型
 * @param[in]      client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 * @param[in]   cb              解析结束后的回调函数
 * @param[in]   cb_param        回调函数的用户自定义参数
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_client_t线程安全，可多线程共享
 */
hdns_status_t hdns_get_result_for_host_async_without_cache(hdns_client_t *client,
                                                           const char *host,
                                                           hdns_query_type_t query_type,
                                                           const char *client_ip,
                                                           hdns_resv_done_callback_pt cb,
                                                           void *cb_param);

/*
 *
 * @brief 批量域名异步解析，不阻塞线程，先查缓存，缓存为空则查询HTTPDNS服务器，直到结果返回或者超时，最后触发函数回调
 *
 * @param[in]      client          客户端实例
 * @param[in]      hosts         待解析的域名列表
 * @param[in]      query_type    请求类型
 *         - HDNS_QUERY_AUTO：根据网络栈自动解析；
 *         - HDNS_QUERY_IPV4：解析IPV4类型；
 *         - HDNS_QUERY_IPV6：解析IPv6类型
 *         - HDNS_QUERY_BOTH：解析IPV4和IPV6类型
 * @param[in]      client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 * @param[in]   cb              解析结束后的回调函数
 * @param[in]   cb_param        回调函数的用户自定义参数
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_client_t线程安全，可多线程共享
 */
hdns_status_t hdns_get_results_for_hosts_async_with_cache(hdns_client_t *client,
                                                          const hdns_list_head_t *hosts,
                                                          hdns_query_type_t query_type,
                                                          const char *client_ip,
                                                          hdns_resv_done_callback_pt cb,
                                                          void *cb_param);

/*
 *
 * @brief 批量域名异步解析，不阻塞线程，查询HTTPDNS服务器，直到结果返回或者超时，最后触发函数回调
 *
 * @param[in]      client          客户端实例
 * @param[in]      hosts         待解析的域名列表
 * @param[in]      query_type    请求类型
 *         - HDNS_QUERY_AUTO：根据网络栈自动解析；
 *         - HDNS_QUERY_IPV4：解析IPV4类型；
 *         - HDNS_QUERY_IPV6：解析IPv6类型
 *         - HDNS_QUERY_BOTH：解析IPV4和IPV6类型
 * @param[in]      client_ip 可选，客户端ip, 默认为接口调用方的出口IP
 * @param[in]   cb              解析结束后的回调函数
 * @param[in]   cb_param        回调函数的用户自定义参数
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 * @note :
 *    - hdns_client_t线程安全，可多线程共享
 */
hdns_status_t hdns_get_results_for_hosts_async_without_cache(hdns_client_t *client,
                                                             const hdns_list_head_t *hosts,
                                                             hdns_query_type_t query_type,
                                                             const char *client_ip,
                                                             hdns_resv_done_callback_pt cb,
                                                             void *cb_param);

/*
 *
 * @brief 从解析列表中随机选择一个ip，双栈时ipv4优先，仅适用于单个域名解析的情况
 *
 * @param[in]      results       已获取的解析结果
 * @param[in]      query_type    请求类型
 *         - HDNS_QUERY_AUTO：根据网络栈自动解析；
 *         - HDNS_QUERY_IPV4：解析IPV4类型；
 *         - HDNS_QUERY_IPV6：解析IPv6类型
 *         - HDNS_QUERY_BOTH：解析IPV4和IPV6类型
 * @param[out]     ip            写入ip的buffer
 * @return  操作状态，0表示成功，否则表示失败
 */
int hdns_select_ip_randomly(hdns_list_head_t *results, hdns_query_type_t query_type, char *ip);

/*
 *
 * @brief 返回解析ip列表中第一个ip，如果开启了ip优选，往往意味着最优ip，仅适用于单个域名解析的情况
 *
 * @param[in]      results       已获取的解析结果
 * @param[in]      query_type    请求类型
 *         - HDNS_QUERY_AUTO：根据网络栈自动解析；
 *         - HDNS_QUERY_IPV4：解析IPV4类型；
 *         - HDNS_QUERY_IPV6：解析IPv6类型
 *         - HDNS_QUERY_BOTH：解析IPV4和IPV6类型
 * @param[out]     ip            写入ip的buffer
 * @return  操作状态，0表示成功，否则表示失败
 */
int hdns_select_first_ip(hdns_list_head_t *results, hdns_query_type_t query_type, char *ip);


/*
 *
 * @brief 返回软件自定义解析中的extra字段
 *
 * @param[in]      results       已获取的解析结果
 * @param[in]      query_type    请求类型
 *         - HDNS_QUERY_AUTO：根据网络栈自动解析；
 *         - HDNS_QUERY_IPV4：解析IPV4类型；
 *         - HDNS_QUERY_IPV6：解析IPv6类型
 *         - HDNS_QUERY_BOTH：解析IPV4和IPV6类型
 * @param[out]     extra      写入extra的buffer
 * @return  操作状态，0表示成功，否则表示失败
 */
int hdns_get_sdns_extra(hdns_list_head_t *results, hdns_query_type_t query_type, char *extra);

/*
 *
 * @brief 设置SDK日志文件路径
 *
 * @param[in]      file_path     日志文件路径
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 */
hdns_status_t hdns_log_set_log_file_path(const char *file_path);


/*
 *
 * @brief 设置SDK日志等级
 *
 * @param[in]      level     日志等级
 *    - HDNS_LOG_OFF    不打开日志
 *    - HDNS_LOG_FATAL  fatal级及以下
 *    - HDNS_LOG_ERROR  error级及以下
 *    - HDNS_LOG_WARN   warn级及以下
 *    - HDNS_LOG_INFO   info级及以下
 *    - HDNS_LOG_DEBUG  debug级及以下
 *    - HDNS_LOG_TRACE  trace级及以下
 */
void hdns_log_set_log_level(hdns_log_level_e level);

/*
 *
 * @brief 清除某域名的本地缓存
 *
 * @param[in]      client     客户端实例
 * @param[in]      host       域名
 * @return  操作状态，如果status的code是0表示成功，否则表示失败，error_msg包含了错误信息
 */
void hdns_remove_host_cache(hdns_client_t *client, const char *host);
/*
 *
 * @brief 清理释放HTTPDNS客户端资源
 *
 * @param[in]      client     客户端实例
 */
void hdns_client_cleanup(hdns_client_t *client);

/*
 *
 * @brief 清理释放HTTPDNS SDK环境
 * @node :
 *  - 该接口并非线程安全接口
 */
void hdns_sdk_cleanup();


HDNS_CPP_END

#endif
