#include "hdns_fstack.h"

hdns_array_header_t *hdns_fstack_create(hdns_pool_t *p, int size)
{
    return apr_array_make(p, size, sizeof(hdns_fstack_item_t));
}

void hdns_fstack_push(hdns_array_header_t *fstack, void *data, hdns_func_u func, int order)
{
    hdns_fstack_item_t *item;

    item = (hdns_fstack_item_t*)apr_array_push(fstack);
    item->data = data;
    item->func = func;
    item->order = order;
}

hdns_fstack_item_t *hdns_fstack_pop(hdns_array_header_t *fstack)
{
    hdns_fstack_item_t *item;    
    
    item = (hdns_fstack_item_t*)apr_array_pop(fstack);
    if (item == NULL) {
        return NULL;
    }

    switch (item->order) {
        case 1:
            item->func.func1(item->data);
            break;
        case 2:
            item->func.func2();
            break;
        case 3:
            item->func.func3(item->data);
            break;
        case 4:
            item->func.func4();
            break;
        default:
            break;
    }
    
    return item;
}

void hdns_fstack_destory(hdns_array_header_t *fstack)
{
    while (hdns_fstack_pop(fstack) != NULL);
}
