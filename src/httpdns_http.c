//
// Created by caogaoshuai on 2024/1/12.
//
#include "httpdns_http.h"
#include <arpa/inet.h>
#include "openssl/ssl.h"
#include "openssl/x509v3.h"
#include "httpdns_memory.h"
#include "httpdns_client_config.h"
#include "httpdns_sds.h"
#include "log.h"

httpdns_http_context_t *httpdns_http_context_new(const char *url, int32_t timeout_ms) {
    if (httpdns_string_is_blank(url)) {
        log_info("create httpdns http context failed, url is blank");
        return NULL;
    }
    HTTPDNS_NEW_OBJECT_IN_HEAP(httpdns_http_ctx, httpdns_http_context_t);
    httpdns_http_ctx->request_url = httpdns_sds_new(url);
    if (timeout_ms <= 0) {
        log_debug("request timeout is less than 0, using default %d", MAX_HTTP_REQUEST_TIMEOUT_MS);
        httpdns_http_ctx->request_timeout_ms = MAX_HTTP_REQUEST_TIMEOUT_MS;
    } else {
        httpdns_http_ctx->request_timeout_ms = timeout_ms;
    }
    httpdns_http_ctx->user_agent = httpdns_sds_new(USER_AGENT);
    return httpdns_http_ctx;
}

int32_t httpdns_http_context_set_private_data(httpdns_http_context_t *http_context, void *private_data) {
    if (NULL == http_context || NULL == private_data) {
        log_info("set private data failed, http context or private data is NULL");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    http_context->private_data = private_data;
    return HTTPDNS_SUCCESS;
}

int32_t httpdns_http_context_set_user_agent(httpdns_http_context_t *http_context, const char *user_agent) {
    if (NULL == http_context || NULL == user_agent) {
        log_info("set user agent failed, http context or user agent is NULL");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    HTTPDNS_SET_STRING_FIELD(http_context, user_agent, user_agent);
    return HTTPDNS_SUCCESS;
}

httpdns_sds_t httpdns_http_context_to_string(const httpdns_http_context_t *http_context) {
    if (NULL == http_context) {
        return httpdns_sds_new("httpdns_http_context_t()");
    }
    httpdns_sds_t dst_str = httpdns_sds_new("httpdns_http_context_t(request_url=");
    httpdns_sds_cat_easily(dst_str, http_context->request_url);
    httpdns_sds_cat_easily(dst_str, ",request_timeout_ms=");
    httpdns_sds_cat_int(dst_str, http_context->request_timeout_ms);
    httpdns_sds_cat_easily(dst_str, ",user_agent=");
    httpdns_sds_cat_easily(dst_str, http_context->user_agent);
    httpdns_sds_cat_easily(dst_str, ",response_body=");
    httpdns_sds_cat_easily(dst_str, http_context->response_body);
    httpdns_sds_cat_easily(dst_str, ",response_status=");
    httpdns_sds_cat_int(dst_str, http_context->response_status);
    httpdns_sds_cat_easily(dst_str, ",response_rt_ms=");
    httpdns_sds_cat_int(dst_str, http_context->response_rt_ms);
    httpdns_sds_cat_easily(dst_str, ")");
    return dst_str;
}

void httpdns_http_context_free(httpdns_http_context_t *http_context) {
    if (NULL == http_context) {
        return;
    }
    if (NULL != http_context->request_url) {
        httpdns_sds_free(http_context->request_url);
    }
    if (NULL != http_context->user_agent) {
        httpdns_sds_free(http_context->user_agent);
    }
    if (NULL != http_context->response_body) {
        httpdns_sds_free(http_context->response_body);
    }
    free(http_context);
}

int32_t httpdns_http_single_exchange(httpdns_http_context_t *http_context) {
    httpdns_list_new_empty_in_stack(http_contexts);
    httpdns_list_add(&http_contexts, http_context, NULL);
    int32_t ret = httpdns_http_multiple_exchange(&http_contexts);
    httpdns_list_free(&http_contexts, NULL);
    return ret;
}

static size_t write_data_callback(void *buffer, size_t size, size_t nmemb, void *write_data) {
    size_t real_size = size * nmemb;
    httpdns_http_context_t *response_ptr = (httpdns_http_context_t *) write_data;
    response_ptr->response_body = httpdns_sds_new_len(buffer, real_size);
    return real_size;
}

static int32_t httpdns_http_context_timeout_cmp(httpdns_http_context_t *ctx1, httpdns_http_context_t *ctx2) {
    if (NULL == ctx1 && NULL == ctx2) {
        return 0;
    }
    if (NULL == ctx1 && NULL != ctx2) {
        return -1;
    }
    if (NULL != ctx1 && NULL == ctx2) {
        return 1;
    }
    return ctx1->request_timeout_ms - ctx2->request_timeout_ms;
}

static int32_t calculate_max_request_timeout(httpdns_list_head_t *http_contexts) {
    httpdns_http_context_t *ctx = httpdns_list_max(http_contexts, to_httpdns_data_cmp_func(httpdns_http_context_timeout_cmp));
    if (ctx->request_timeout_ms < MIN_HTTP_REQUEST_TIMEOUT_MS) {
        log_info("request timeout is too small, use %d", MIN_HTTP_REQUEST_TIMEOUT_MS);
        return MIN_HTTP_REQUEST_TIMEOUT_MS;
    }
    if (ctx->request_timeout_ms > MAX_HTTP_REQUEST_TIMEOUT_MS) {
        log_info("request timeout is too big, use %d", MAX_HTTP_REQUEST_TIMEOUT_MS);
        return MAX_HTTP_REQUEST_TIMEOUT_MS;
    }
    return ctx->request_timeout_ms;
}

/**
 * 对于mac环境，目前无法在请求前进行证书校验，只能请求后校验
 */
static int32_t ssl_cert_verify(CURL *curl) {
    struct curl_certinfo *certinfo;
    CURLcode res = curl_easy_getinfo(curl, CURLINFO_CERTINFO, &certinfo);
    if (res || !certinfo) {
        log_error("get cert information from response failed");
        return HTTPDNS_CERT_VERIFY_FAILED;
    }
    httpdns_list_head_t host_names;
    httpdns_list_init(&host_names);
    for (int i = 0; i < certinfo->num_of_certs; i++) {
        for (struct curl_slist *slist = certinfo->certinfo[i]; slist; slist = slist->next) {
            if (strncmp(slist->data, CERT_PEM_NAME, strlen(CERT_PEM_NAME)) != 0) {
                continue;
            }
            const char *pem_cert = slist->data + strlen(CERT_PEM_NAME);
            BIO *bio = BIO_new(BIO_s_mem());
            BIO_puts(bio, pem_cert);
            X509 *cert = PEM_read_bio_X509(bio, NULL, 0, NULL);
            if (!cert) {
                goto free_cert_bio;
            }
            // 提取 Common Name (CN)
            X509_NAME *subject = X509_get_subject_name(cert);
            int index = X509_NAME_get_index_by_NID(subject, NID_commonName, -1);
            if (index < 0) {
                goto free_cert_bio;
            }
            X509_NAME_ENTRY *entry = X509_NAME_get_entry(subject, index);
            ASN1_STRING *data = X509_NAME_ENTRY_get_data(entry);
            unsigned char *cn;
            ASN1_STRING_to_UTF8(&cn, data);
            if (strcmp(SSL_VERIFY_HOST, cn) != 0) {
                OPENSSL_free(cn);
                goto free_cert_bio;
            }
            httpdns_list_add(&host_names, cn, httpdns_string_clone_func);
            OPENSSL_free(cn);

            // 提取 Subject Alternative Name (SAN)
            index = X509_get_ext_by_NID(cert, NID_subject_alt_name, -1);
            if (index < 0) {
                goto free_cert_bio;
            }
            X509_EXTENSION *san_extension = X509_get_ext(cert, index);
            if (!san_extension) {
                goto free_cert_bio;
            }
            GENERAL_NAMES *san_names = (GENERAL_NAMES *) X509V3_EXT_d2i(san_extension);
            int san_count = sk_GENERAL_NAME_num(san_names);
            for (int inner_i = 0; inner_i < san_count; ++inner_i) {
                GENERAL_NAME *san_entry = sk_GENERAL_NAME_value(san_names, inner_i);
                if (!san_entry) {
                    continue;
                }
                // 根据 SAN 类型处理不同数据
                if (san_entry->type == GEN_DNS) {
                    const char *dns_name = (const char *) ASN1_STRING_get0_data(san_entry->d.dNSName);
                    httpdns_list_add(&host_names, dns_name, httpdns_string_clone_func);
                    continue;
                }
                if (san_entry->type == GEN_IPADD) {
                    unsigned char *ip_addr = NULL;
                    char ip_str[INET6_ADDRSTRLEN];
                    ip_addr = (unsigned char *) ASN1_STRING_get0_data(san_entry->d.iPAddress);
                    if (ASN1_STRING_length(san_entry->d.iPAddress) == 4) {
                        // IPv4 地址
                        inet_ntop(AF_INET, ip_addr, ip_str, sizeof(ip_str));
                    } else if (ASN1_STRING_length(san_entry->d.iPAddress) == 16) {
                        // IPv6 地址
                        inet_ntop(AF_INET6, ip_addr, ip_str, sizeof(ip_str));
                    }
                    httpdns_list_add(&host_names, ip_str, httpdns_string_clone_func);
                }
            }
            sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);
            free_cert_bio:
            X509_free(cert);
            BIO_free(bio);
        }
    }
    httpdns_sds_t host_names_str = httpdns_list_to_string(&host_names, NULL);
    log_debug("get host name from https cert is %s", host_names_str);
    bool is_domain_matched = httpdns_list_contain(&host_names, SSL_VERIFY_HOST, httpdns_string_cmp_func);
    httpdns_list_free(&host_names, httpdns_string_free_func);
    if (!is_domain_matched) {
        log_error("verify https cert failed, cert hosts is %s, expected host is %s", host_names_str, SSL_VERIFY_HOST);
    }
    httpdns_sds_free(host_names_str);
    return is_domain_matched ? HTTPDNS_SUCCESS : HTTPDNS_CERT_VERIFY_FAILED;
}

static CURLcode ssl_ctx_callback(CURL *curl, void *ssl_ctx, void *user_param) {
    (void) curl;
    (void) user_param;
    X509_VERIFY_PARAM *param = SSL_CTX_get0_param(ssl_ctx);
    if (NULL == param) {
        X509_VERIFY_PARAM *new_param = X509_VERIFY_PARAM_new();
        X509_VERIFY_PARAM_set1_host(new_param, SSL_VERIFY_HOST, strlen(SSL_VERIFY_HOST));
        SSL_CTX_set1_param(ssl_ctx, new_param);
        X509_VERIFY_PARAM_free(new_param);
    } else {
        X509_VERIFY_PARAM_set1_host(param, SSL_VERIFY_HOST, strlen(SSL_VERIFY_HOST));
    }
    return CURLE_OK;
}

int32_t httpdns_http_multiple_exchange(httpdns_list_head_t *http_contexts) {
    if (httpdns_list_is_empty(http_contexts)) {
        log_info("multiple exchange failed, http_contexts is empty");
        return HTTPDNS_PARAMETER_EMPTY;
    }
    CURLM *multi_handle = curl_multi_init();
    httpdns_list_for_each_entry(http_context_cursor, http_contexts) {
        httpdns_http_context_t *http_context = http_context_cursor->data;
        CURL *handle = curl_easy_init();
        curl_easy_setopt(handle, CURLOPT_URL, http_context->request_url);
        curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, http_context->request_timeout_ms);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
        bool using_https = IS_HTTPS_SCHEME(http_context->request_url);
        if (using_https) {
            curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
            curl_easy_setopt(handle, CURLOPT_CERTINFO, 1L);
        }
        curl_easy_setopt(handle, CURLOPT_PRIVATE, http_context);
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data_callback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, http_context);
        curl_easy_setopt(handle, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(handle, CURLOPT_USERAGENT, http_context->user_agent);
        /**
         * 这里curl本身的host name校验关闭，但并不关闭openssl的校验
         *
         * 即 https://github.com/curl/curl/blob/master/lib/vtls/openssl.c
         *
         * 文件中的Curl_ossl_verifyhost方法不调用
         */
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
#ifdef __linux__
        //仅支持OpenSSL, wolfSSL, mbedTLS or BearSSL，对于mac环境的LibreSSL不支持
        //https://curl.se/libcurl/c/CURLOPT_SSL_CTX_FUNCTION.html
       curl_easy_setopt(handle, CURLOPT_SSL_CTX_FUNCTION, ssl_ctx_callback);
#endif
        curl_multi_add_handle(multi_handle, handle);
    }
    int32_t max_timeout_ms = calculate_max_request_timeout(http_contexts);
    int running_handles;
    do {
        CURLMcode mc = curl_multi_perform(multi_handle, &running_handles);
        if (mc != CURLM_OK) {
            log_info("curl_multi_perform exception, CURLMcode is %d", mc);
            break;
        }
        curl_multi_wait(multi_handle, NULL, 0, max_timeout_ms, NULL);
    } while (running_handles);

    int msgq = 0;
    CURLMsg *msg;
    while ((msg = curl_multi_info_read(multi_handle, &msgq)) != NULL) {
        if (msg->msg == CURLMSG_DONE) {
            httpdns_http_context_t *http_context;
            curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &http_context);
            if (NULL != http_context) {
                bool using_https = IS_HTTPS_SCHEME(http_context->request_url);
#ifdef __APPLE__
                bool very_https_cert_success = using_https && (ssl_cert_verify(msg->easy_handle) == HTTPDNS_SUCCESS);
#else
                bool very_https_cert_success = true;
#endif
                if (!using_https || very_https_cert_success) {
                    curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &http_context->response_status);
                    double total_time_sec;
                    curl_easy_getinfo(msg->easy_handle, CURLINFO_TOTAL_TIME, &total_time_sec);
                    http_context->response_rt_ms = (int32_t) (total_time_sec * 1000.0);
                } else {
                    curl_multi_cleanup(multi_handle);
                    log_error("multiple exchange failed, verify https cert failed");
                    return HTTPDNS_CERT_VERIFY_FAILED;
                }
            }
            curl_multi_remove_handle(multi_handle, msg->easy_handle);
            curl_easy_cleanup(msg->easy_handle);
        }
    }
    curl_multi_cleanup(multi_handle);
    return HTTPDNS_SUCCESS;
}
