//
// Created by cagaoshuai on 2024/1/12.
//
#include "httpdns_http.h"
#include <arpa/inet.h>
#include "openssl/ssl.h"
#include "openssl/x509v3.h"

httpdns_http_request_t *create_httpdns_http_request(char *url, int32_t timeout_ms, char *cache_key) {
    if (IS_BLANK_SDS(url) || timeout_ms <= 0) {
        return NULL;
    }
    httpdns_http_request_t *request = (httpdns_http_request_t *) malloc(sizeof(httpdns_http_request_t));
    memset(request, 0, sizeof(httpdns_http_request_t));
    request->timeout_ms = timeout_ms;
    if (!IS_BLANK_SDS(url)) {
        request->url = sdsnew(url);
    }
    if (!IS_BLANK_SDS(cache_key)) {
        request->cache_key = sdsnew(cache_key);
    }
    return request;
}

httpdns_http_request_t *clone_httpdns_http_request(const httpdns_http_request_t *request) {
    if (NULL == request) {
        return NULL;
    }
    return create_httpdns_http_request(request->url, request->timeout_ms, request->cache_key);
}

void destroy_httpdns_http_request(httpdns_http_request_t *request) {
    if (NULL == request) {
        return;
    }
    if (NULL != request->url) {
        sdsfree(request->url);
    }
    if (NULL != request->cache_key) {
        sdsfree(request->cache_key);
    }
    free(request);
}


httpdns_http_response_t *create_httpdns_http_response(char *url, char *cache_key) {
    if (IS_BLANK_SDS(url)) {
        return NULL;
    }
    httpdns_http_response_t *response = (httpdns_http_response_t *) malloc(sizeof(httpdns_http_response_t));
    memset(response, 0, sizeof(httpdns_http_response_t));
    if (!IS_BLANK_SDS(url)) {
        response->url = sdsnew(url);
    }
    if (!IS_BLANK_SDS(cache_key)) {
        response->cache_key = sdsnew(cache_key);
    }
    return response;
}

httpdns_http_response_t *clone_httpdns_http_response(const httpdns_http_response_t *origin_response) {
    if (NULL == origin_response) {
        return NULL;
    }
    httpdns_http_response_t *response = (httpdns_http_response_t *) malloc(sizeof(httpdns_http_response_t));
    memset(response, 0, sizeof(httpdns_http_response_t));
    if (!IS_BLANK_SDS(origin_response->url)) {
        response->url = sdsnew(origin_response->url);
    }
    if (!IS_BLANK_SDS(origin_response->cache_key)) {
        response->cache_key = sdsnew(origin_response->cache_key);
    }
    response->http_status = origin_response->http_status;
    response->body = sdsnew(origin_response->body);
    response->total_time_ms = origin_response->total_time_ms;
    return response;
}

void destroy_httpdns_http_response(httpdns_http_response_t *response) {
    if (NULL == response) {
        return;
    }
    if (NULL != response->body) {
        sdsfree(response->body);
    }
    if (NULL != response->url) {
        sdsfree(response->url);
    }
    if (NULL != response->cache_key) {
        sdsfree(response->cache_key);
    }
    free(response);
}

void destroy_httpdns_http_responses(struct list_head *responses) {
    httpdns_list_free(responses, DATA_FREE_FUNC(destroy_httpdns_http_response));
}

void destroy_httpdns_http_requests(struct list_head *requests) {
    httpdns_list_free(requests, DATA_FREE_FUNC(destroy_httpdns_http_request));
}


int32_t httpdns_http_single_request_exchange(httpdns_http_request_t *request, httpdns_http_response_t **response) {
    struct list_head requests;
    struct list_head responses;
    httpdns_list_init(&requests);
    httpdns_list_init(&responses);
    httpdns_list_add(&requests, request, DATA_CLONE_FUNC(clone_httpdns_http_request));
    int32_t ret = httpdns_http_multiple_request_exchange(&requests, &responses);
    httpdns_list_free(&requests, DATA_FREE_FUNC(destroy_httpdns_http_request));
    if (ret != HTTPDNS_SUCCESS || IS_EMPTY_LIST(&responses)) {
        return HTTPDNS_CORRECT_RESPONSE_EMPTY;
    }
    *response = clone_httpdns_http_response(httpdns_list_get(&responses, 0));
    httpdns_list_free(&responses, DATA_FREE_FUNC(destroy_httpdns_http_response));
    return HTTPDNS_SUCCESS;
}

static size_t write_data_callback(void *buffer, size_t size, size_t nmemb, void *write_data) {
    size_t real_size = size * nmemb;
    httpdns_http_response_t *response_ptr = (httpdns_http_response_t *) write_data;
    response_ptr->body = sdsnewlen(buffer, real_size);
    return real_size;
}

/**
 * 对于mac环境，目前无法在请求前进行证书校验，只能请求后校验
 */
static int32_t ssl_cert_verify(CURL *curl) {
    struct curl_certinfo *certinfo;
    CURLcode res = curl_easy_getinfo(curl, CURLINFO_CERTINFO, &certinfo);
    if (res || !certinfo) {
        return HTTPDNS_CERT_VERIFY_FAILED;
    }
    struct list_head host_names;
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
            httpdns_list_add(&host_names, cn, STRING_CLONE_FUNC);
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
                    httpdns_list_add(&host_names, dns_name, STRING_CLONE_FUNC);
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
                    httpdns_list_add(&host_names, ip_str, STRING_CLONE_FUNC);
                }
            }
            sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);
            free_cert_bio:
            X509_free(cert);
            BIO_free(bio);
        }
    }
    bool is_domain_matched = httpdns_list_contain(&host_names, SSL_VERIFY_HOST, STRING_CMP_FUNC);
    httpdns_list_free(&host_names, STRING_FREE_FUNC);
    return is_domain_matched ? HTTPDNS_SUCCESS : HTTPDNS_CERT_VERIFY_FAILED;
}

static CURLcode ssl_ctx_callback(CURL *curl, void *ssl_ctx, void *user_param) {
    (void) curl;
    (void) user_param;
    (void) ssl_ctx;
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

int32_t httpdns_http_multiple_request_exchange(struct list_head *requests, struct list_head *responses) {
    if (NULL == responses || NULL == requests) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    size_t request_num = httpdns_list_size(requests);
    if (request_num <= 0) {
        return HTTPDNS_PARAMETER_EMPTY;
    }
    CURLM *multi_handle = curl_multi_init();
    int32_t max_timeout_ms = MULTI_HANDLE_TIMEOUT_MS;
    for (int i = 0; i < request_num; i++) {
        httpdns_http_request_t *request = httpdns_list_get(requests, i);
        httpdns_http_response_t *response = create_httpdns_http_response(request->url, request->cache_key);
        CURL *handle = curl_easy_init();
        curl_easy_setopt(handle, CURLOPT_URL, request->url);
        curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, request->timeout_ms);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
        /**
         * 这里curl本身的host name校验关闭，但并不关闭openssl的校验
         *
         * 即 https://github.com/curl/curl/blob/master/lib/vtls/openssl.c
         *
         * 文件中的Curl_ossl_verifyhost方法不调用
         */
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(handle, CURLOPT_PRIVATE, response);
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data_callback);
#ifdef __linux__
        //仅支持OpenSSL, wolfSSL, mbedTLS or BearSSL，对于mac环境的LibreSSL不支持
        //https://curl.se/libcurl/c/CURLOPT_SSL_CTX_FUNCTION.html
       curl_easy_setopt(handle, CURLOPT_SSL_CTX_FUNCTION, ssl_ctx_callback);
#endif
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, response);
        curl_easy_setopt(handle, CURLOPT_CERTINFO, 1L);
        curl_easy_setopt(handle, CURLOPT_VERBOSE, 0L);

        curl_multi_add_handle(multi_handle, handle);
        if (max_timeout_ms < request->timeout_ms) {
            max_timeout_ms = request->timeout_ms;
        }
    }
    int still_running;
    do {
        CURLMcode mc = curl_multi_perform(multi_handle, &still_running);
        if (mc != CURLM_OK) {
            break;
        }
        curl_multi_wait(multi_handle, NULL, 0, max_timeout_ms, NULL);
    } while (still_running);

    int msgq = 0;
    CURLMsg *msg;
    while ((msg = curl_multi_info_read(multi_handle, &msgq)) != NULL) {
        if (msg->msg == CURLMSG_DONE) {
            httpdns_http_response_t *resonse;
            curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &resonse);
            if (NULL != resonse) {
                bool using_https = IS_HTTPS_SCHEME(resonse->url);
#ifdef __APPLE__
                bool very_https_cert_success = (ssl_cert_verify(msg->easy_handle) == HTTPDNS_SUCCESS);
#else
                bool very_https_cert_success = true;
#endif
                if (!using_https || very_https_cert_success) {
                    curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &resonse->http_status);
                    double total_time_sec;
                    curl_easy_getinfo(msg->easy_handle, CURLINFO_TOTAL_TIME, &total_time_sec);
                    resonse->total_time_ms = (int32_t) (total_time_sec * 1000.0);
                    httpdns_list_add(responses, resonse, DATA_CLONE_FUNC(clone_httpdns_http_response));
                }
                destroy_httpdns_http_response(resonse);
            }
            curl_multi_remove_handle(multi_handle, msg->easy_handle);
            curl_easy_cleanup(msg->easy_handle);
        }
    }
    if (NULL != multi_handle) {
        curl_multi_cleanup(multi_handle);
    }
    return IS_EMPTY_LIST(responses) ? HTTPDNS_CORRECT_RESPONSE_EMPTY : HTTPDNS_SUCCESS;
}
