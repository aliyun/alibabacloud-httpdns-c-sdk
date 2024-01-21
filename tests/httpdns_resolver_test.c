//
// Created by cagaoshuai on 2024/1/20.
//
#include "httpdns_resolver.h"

int main(void) {
    httpdns_config_t *config = create_httpdns_config();
    httpdns_config_set_account_id(config, "139450");
    httpdns_config_set_using_https(config, true);
    httpdns_resolver_t* resolver = create_httpdns_resolver(config);
    httpdns_resolve_task_t *task = create_httpdns_resolve_task(resolver);
    httpdns_resolve_request_t* request = create_httpdns_resolve_request("www.aliyun.com", TYPE_A, NULL);
    httpdns_resolve_task_add_request(task, request);
    resolve(task);
    httpdns_list_print(&task->results, DATA_PRINT_FUNC(print_httpdns_resolve_result));
    httpdns_list_free(&task->results, DATA_FREE_FUNC(destroy_httpdns_resolve_result));
    destroy_httpdns_resolver(resolver);
    destroy_httpdns_resolve_task(task);
    destroy_httpdns_config(config);
    return 0;
}