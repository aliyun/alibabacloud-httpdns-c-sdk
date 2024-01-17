//
// Created by cagaoshuai on 2024/1/17.
//
#include "httpdns_scheduler.h"
#include "httpdns_global_config.h"

static void print_resolve_servers(struct list_head *servers) {
    size_t server_size = httpdns_list_size(servers);
    for (int i = 0; i < server_size; i++) {
        httpdns_resolve_server_t *server = httpdns_list_get(servers, i);
        printf("Server %d: host %s, weight %d\n", i, server->server, server->weight);
    }
}

static int32_t test_refresh_resolve_servers() {
    httpdns_config_t *config = create_httpdns_config();
    httpdns_config_set_account_id(config, "100000");
    httpdns_config_set_using_https(config, true);
    httpdns_scheduler_t *scheduler = create_httpdns_scheduler(config);
    int32_t ret = httpdns_scheduler_refresh_resolve_servers(scheduler);
    print_resolve_servers(&scheduler->ipv4_resolve_servers);
    print_resolve_servers(&scheduler->ipv6_resolve_servers);
    destroy_httpdns_config(config);
    destroy_httpdns_scheduler(scheduler);
    return ret ? HTTPDNS_FAILURE : HTTPDNS_SUCCESS;
}




int main(void) {
    init_httpdns_sdk();
    test_refresh_resolve_servers();
    cleanup_httpdns_sdk();
    return 0;
}