//
// Created by cagaoshuai on 2024/1/12.
//
#include "httpdns_http.h"
#include <arpa/inet.h>
#include "openssl/ssl.h"
#include "openssl/x509v3.h"

httpdns_http_request_t *create_httpdns_http_request(char *url, int64_t timeout_ms, char *cache_key) {
    if (IS_BLANK_SDS(url) || timeout_ms <= 0) {
        return NULL;
    }
    httpdns_http_request_t *request = (httpdns_http_request_t *) malloc(sizeof(httpdns_http_request_t));
    memset(request, 0, sizeof(httpdns_http_request_t));
    request->timeout_ms = timeout_ms;
    request->url = sdsnew(url);
    if (NULL != cache_key) {
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
    response->url = sdsnew(url);
    if (!IS_BLANK_SDS(cache_key)) {
        response->cache_key = sdsnew(cache_key);
    }
    return response;
}

httpdns_http_response_t *clone_httpdns_http_response(const httpdns_http_response_t *origin_response) {
    if (NULL != origin_response) {
        return NULL;
    }
    httpdns_http_response_t *response = (httpdns_http_response_t *) malloc(sizeof(httpdns_http_response_t));
    memset(response, 0, sizeof(httpdns_http_response_t));
    if (!IS_BLANK_SDS(origin_response->url)) {
        response->url = sdsdup(origin_response->url);
    }
    if (!IS_BLANK_SDS(origin_response->cache_key)) {
        response->cache_key = sdsdup(origin_response->cache_key);
    }
    response->http_status = origin_response->http_status;
    response->body = sdsdup(origin_response->body);
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


int32_t httpdns_http_single_request_exchange(httpdns_http_request_t *request, httpdns_http_response_t**response) {
    struct list_head requests;
    struct list_head responses;
    httpdns_list_init(&requests);
    httpdns_list_init(&responses);
    httpdns_list_add(&requests, request, DATA_CLONE_FUNC(clone_httpdns_http_request));
    int32_t ret = httpdns_http_multiple_request_exchange(&requests, &responses);
    if (ret !=HTTPDNS_SUCCESS) {
        return ret;
    }
    *response  = httpdns_list_get(&responses, 0);
    return HTTPDNS_SUCCESS;
}

static size_t write_data_callback(void *buffer, size_t size, size_t nmemb, void *write_data) {
    size_t real_size = size * nmemb;
    httpdns_http_response_t *response_ptr = (httpdns_http_response_t *) write_data;
    response_ptr->body = sdsnewlen(buffer, real_size);
    return real_size;
}

static int32_t ssl_cert_verify(CURL *curl) {
    struct curl_certinfo *certinfo;
    CURLcode res = curl_easy_getinfo(curl, CURLINFO_CERTINFO, &certinfo);

    if(!res && certinfo) {
        for(int i = 0; i < certinfo->num_of_certs; i++) {
            for(struct curl_slist *slist = certinfo->certinfo[i]; slist; slist = slist->next) {
                if(strncmp(slist->data, "Cert:", 5) == 0) {
                    // PEM格式的证书字符串位于"Cert:"后面
                    const char *pem_cert = slist->data + 5;

                    BIO *bio = BIO_new(BIO_s_mem());
                    BIO_puts(bio, pem_cert);

                    X509 *cert = PEM_read_bio_X509(bio, NULL, 0, NULL);
                    if(cert) {
                        // 提取 Common Name (CN)
                        X509_NAME *subject = X509_get_subject_name(cert);
                        int index = X509_NAME_get_index_by_NID(subject, NID_commonName, -1);
                        if(index >= 0) {
                            X509_NAME_ENTRY *entry = X509_NAME_get_entry(subject, index);
                            ASN1_STRING *data = X509_NAME_ENTRY_get_data(entry);
                            unsigned char *cn;
                            ASN1_STRING_to_UTF8(&cn, data);
                            printf("Common Name: %s\n", cn);
                            OPENSSL_free(cn);
                        }

                        // 提取 Subject Alternative Name (SAN)
                        index = X509_get_ext_by_NID(cert, NID_subject_alt_name, -1);
                        if(index >0){
                            X509_EXTENSION* san_extension = X509_get_ext(cert, index);
                            if (san_extension) {
                                GENERAL_NAMES* san_names = (GENERAL_NAMES*)X509V3_EXT_d2i(san_extension);
                                int san_count = sk_GENERAL_NAME_num(san_names);
                                for (int inner_i = 0; inner_i < san_count; ++inner_i) {
                                    GENERAL_NAME* san_entry = sk_GENERAL_NAME_value(san_names, inner_i);
                                    if (!san_entry) {
                                        continue;
                                    }

                                    // 根据 SAN 类型处理不同数据
                                    if (san_entry->type == GEN_DNS) {
                                        const char *dns_name = (const char *)ASN1_STRING_get0_data(san_entry->d.dNSName);
                                        printf("DNS: %s\n", dns_name);
                                    } else if(san_entry->type == GEN_IPADD) {
                                        unsigned char *ip_addr = NULL;
                                        char ip_str[INET6_ADDRSTRLEN];
                                        ip_addr = (unsigned char *)ASN1_STRING_get0_data(san_entry->d.iPAddress);
                                        if (ASN1_STRING_length(san_entry->d.iPAddress) == 4) {
                                            // IPv4 地址
                                            inet_ntop(AF_INET, ip_addr, ip_str, sizeof(ip_str));
                                        } else if (ASN1_STRING_length(san_entry->d.iPAddress) == 16) {
                                            // IPv6 地址
                                            inet_ntop(AF_INET6, ip_addr, ip_str, sizeof(ip_str));
                                        }
                                        printf("IP Address: %s\n", ip_str);
                                    }
                                    // 可以添加处理其他类型的代码，如 IP 地址 (GEN_IPADD)
                                }

                                // 释放 GENERAL_NAMES 结构占用的内存
                                sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);
                            }
                        }



                        int ext_count = X509_get_ext_count(cert);
                        for(int j = 0; j < ext_count; j++) {
                            X509_EXTENSION *ext = X509_get_ext(cert, j);
                            ASN1_OBJECT *obj = X509_EXTENSION_get_object(ext);
                            printf("%d\n", OBJ_obj2nid(obj));
                            if(OBJ_obj2nid(obj) == NID_subject_alt_name) {
                                GENERAL_NAMES *names = X509V3_EXT_d2i(ext);
                                if(names) {
                                    int name_count = sk_GENERAL_NAME_num(names);

                                    for(int k = 0; k < name_count; k++) {
                                        GENERAL_NAME *name = sk_GENERAL_NAME_value(names, k);
                                        if(name->type == GEN_DNS) {
                                            char *dns_name = (char *)ASN1_STRING_get0_data(name->d.dNSName);
                                            printf("SAN: %s\n", dns_name);
                                        }
                                    }
                                    GENERAL_NAMES_free(names);
                                }
                            }
                        }
                        X509_free(cert);
                    }
                    BIO_free(bio);
                }
            }
        }
    }

//    struct curl_certinfo *certinfo;
//    CURLcode res = curl_easy_getinfo(curl, CURLINFO_CERTINFO, &certinfo);
//    if (res != CURLE_OK || !certinfo || !certinfo->certinfo) {
//        return HTTPDNS_CERT_VERIFY_FAILED;
//    }
//    for (int i = 0; i < certinfo->num_of_certs; i++) {
//        for (struct curl_slist *slist = certinfo->certinfo[i]; slist; slist = slist->next) {
//            if (strstr(slist->data, "Subject:")) {
//                const char *cn = strstr(slist->data, "CN = ");
//                if (cn) {
//                    const char *domain_start = cn + 3;
//                    if (strncmp(domain_start, SSL_VERIFY_HOST, strlen(SSL_VERIFY_HOST)) == 0) {
//                        return HTTPDNS_SUCCESS;
//                    }
//                }
//            }
//        }
//    }
    return HTTPDNS_SUCCESS;
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
    int64_t max_timeout_ms = MULTI_HANDLE_TIMEOUT_MS;
    for (int i = 0; i < request_num; i++) {
        httpdns_http_request_t *request = httpdns_list_get(requests, i);
        httpdns_http_response_t *response = create_httpdns_http_response(request->url, request->cache_key);
        CURL *handle = curl_easy_init();
        curl_easy_setopt(handle, CURLOPT_URL, request->url);
        curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, request->timeout_ms);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(handle, CURLOPT_PRIVATE, response);
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data_callback);
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
                bool very_https_cert_success = (ssl_cert_verify(msg->easy_handle) == HTTPDNS_SUCCESS);
                if ((!using_https) || (using_https && very_https_cert_success)) {
                    curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &resonse->http_status);
                    double total_time_sec;
                    curl_easy_getinfo(msg->easy_handle, CURLINFO_TOTAL_TIME, &total_time_sec);
                    resonse->total_time_ms = (int64_t) (total_time_sec * 1000.0);
                    httpdns_list_add(responses, resonse, DATA_CLONE_FUNC(clone_httpdns_http_response));
                    destroy_httpdns_http_response(resonse);
                }
            }
            curl_multi_remove_handle(multi_handle, msg->easy_handle);
            curl_easy_cleanup(msg->easy_handle);
        }
    }
    return HTTPDNS_SUCCESS;
}
