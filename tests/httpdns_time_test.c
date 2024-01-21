//
// Created by cagaoshuai on 2024/1/20.
//

#include "httpdns_time.h"


int main(void) {
    struct timespec now = httpdns_time_now();
    char time_str[100];
    httpdns_time_to_string(now, time_str, 100);
    return 0;
}