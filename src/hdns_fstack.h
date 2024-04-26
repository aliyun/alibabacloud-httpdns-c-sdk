#ifndef HDNS_C_SDK_HDNS_FSTACK_H
#define HDNS_C_SDK_HDNS_FSTACK_H

#include "hdns_define.h"

HDNS_CPP_START

typedef void (*hdns_func1_pt)(void*);
typedef void (*hdns_func2_pt)();
typedef int (*hdns_func3_pt)(void*);
typedef int (*hdns_func4_pt)();

typedef union hdns_func_u {
    hdns_func1_pt func1;
    hdns_func2_pt func2;
    hdns_func3_pt func3;
    hdns_func4_pt func4;
} hdns_func_u;

typedef struct hdns_fstack_item_t {
    void *data;
    hdns_func_u func;
    int order;
} hdns_fstack_item_t;

hdns_array_header_t *hdns_fstack_create(hdns_pool_t *p, int size);

hdns_fstack_item_t *hdns_fstack_pop(hdns_array_header_t *fstack);

void hdns_fstack_destory(hdns_array_header_t *fstack);

void hdns_fstack_push(hdns_array_header_t *fstack, void *data, hdns_func_u func, int order);

HDNS_CPP_END

#endif
