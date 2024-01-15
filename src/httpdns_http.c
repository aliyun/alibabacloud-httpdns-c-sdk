//
// Created by cagaoshuai on 2024/1/12.
//
#include "httpdns_http.h"

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


httpdns_http_response_t *create_httpdns_http_response(char *url) {
    if (IS_BLANK_SDS(url)) {
        return NULL;
    }
    httpdns_http_response_t *response = (httpdns_http_response_t *) malloc(sizeof(httpdns_http_response_t));
    memset(response, 0, sizeof(httpdns_http_response_t));
    response->url = sdsnew(url);
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
    httpdns_list_free(responses, (data_free_function_ptr_t) destroy_httpdns_http_response);
}

void destroy_httpdns_http_requests(struct list_head *requests) {
    httpdns_list_free(requests, (data_free_function_ptr_t) destroy_httpdns_http_request);
}


httpdns_http_response_t *httpdns_http_single_request_exchange(httpdns_http_request_t *request) {
    struct list_head requests;
    httpdns_list_init(&requests);
    httpdns_list_add(&requests, request);
    struct list_head responses = httpdns_http_multiple_request_exchange(&requests);
    if (httpdns_list_size(&responses) <= 0) {
        return NULL;
    }
    httpdns_list_node_t *node = httpdns_list_get(&responses, 0);
    if (NULL != node) {
        return node->data;
    }
    return NULL;
}

static size_t write_data_callback(void *buffer, size_t size, size_t nmemb, void *write_data) {
    size_t realsize = size * nmemb;
    httpdns_http_response_t *response_ptr = (httpdns_http_response_t *) write_data;
    memset(response_ptr, 0, sizeof(httpdns_http_response_t));
    response_ptr->body = sdsnewlen(buffer, realsize);
    return realsize;
}

static CURLcode ssl_verify_callback(CURL *curl, void *sslctx, void *parm) {
    const char *expected_domain = SSL_VERIFY_HOST;
    struct curl_certinfo *certinfo;
    CURLcode res = curl_easy_getinfo(curl, CURLINFO_CERTINFO, &certinfo);
    if (res != CURLE_OK || !certinfo || !certinfo->certinfo) {
        return CURLE_SSL_CERTPROBLEM;
    }
    for (int i = 0; i < certinfo->num_of_certs; i++) {
        for (struct curl_slist *slist = certinfo->certinfo[i]; slist; slist = slist->next) {
            if (strstr(slist->data, "Subject:")) {
                const char *cn = strstr(slist->data, "CN=");
                if (cn) {
                    const char *domain_start = cn + 3;
                    if (strncmp(domain_start, expected_domain, strlen(expected_domain)) == 0) {
                        return CURLE_OK;
                    }
                }
            }
        }
    }
    return CURLE_PEER_FAILED_VERIFICATION;
}


struct list_head httpdns_http_multiple_request_exchange(struct list_head *requests) {
    struct list_head response_head = {NULL, NULL};
    size_t request_num = httpdns_list_size(requests);
    if (request_num <= 0) {
        return response_head;
    }
    CURLM *multi_handle = curl_multi_init();
    int max_timeout_ms = MULTI_HANDLE_TIMEOUT_MS;
    for (int i = 0; i < request_num; i++) {
        httpdns_list_node_t *node = httpdns_list_get(requests, i);
        httpdns_http_request_t *request = node->data;
        httpdns_http_response_t *response_ptr = create_httpdns_http_response(sdsnew(request->url));
        response_ptr->cache_key = sdsdup(request->cache_key);
        CURL *handle = curl_easy_init();
        curl_easy_setopt(handle, CURLOPT_URL, request->url);
        curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, request->timeout_ms);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(handle, CURLOPT_PRIVATE, response_ptr);
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data_callback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, response_ptr);
        curl_easy_setopt(handle, CURLOPT_SSL_CTX_FUNCTION, ssl_verify_callback);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYSTATUS, 1L);
        curl_easy_setopt(handle, CURLOPT_CERTINFO, 1L);
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
            httpdns_http_response_t *resonse_ptr;
            curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &resonse_ptr);
            if (NULL != resonse_ptr) {
                curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &resonse_ptr->http_status);
                double total_time_sec;
                curl_easy_getinfo(msg->easy_handle, CURLINFO_TOTAL_TIME, &total_time_sec);
                resonse_ptr->total_time_ms = (int64_t) (total_time_sec * 1000.0);
                httpdns_list_add(&response_head, resonse_ptr);
            }
            curl_multi_remove_handle(multi_handle, msg->easy_handle);
            curl_easy_cleanup(msg->easy_handle);
        }
    }
    return response_head;
}
