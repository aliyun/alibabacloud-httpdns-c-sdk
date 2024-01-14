//
// Created by cagaoshuai on 2024/1/14.
//

#include<stdio.h>
#include "net_stack_detector.h"

int main(int argc, char *argv[])
{
    net_stack_detector_t* detector = create_net_stack_detector();
    net_stack_type_t  net_stack_type = get_net_stack_type(detector);
    if(HAVE_IPV4_NET_TYPE(net_stack_type)) {
        printf("have ipv4\n");
    }
    if(HAVE_IPV6_NET_TYPE(net_stack_type)) {
        printf("have ipv6\n");
    }
    destroy_net_stack_detector(detector);
    return 0;
}
