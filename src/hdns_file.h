//
// Created by caogaoshuai on 2024/6/25.
//

#ifndef HDNS_C_SDK_HDNS_FILE_H
#define HDNS_C_SDK_HDNS_FILE_H

#include "hdns_define.h"
#include "hdns_list.h"
#include "hdns_string.h"

int32_t hdns_file_create_dir(const char *dir_path);

int32_t hdns_file_write(const char *file_path, const char *content);

char *hdns_file_read(const char *file_path, apr_pool_t *pool);

#endif
