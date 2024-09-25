//
// Created by caogaoshuai on 2024/6/25.
//
#include "hdns_file.h"
#include "hdns_log.h"
#include "hdns_buf.h"

int32_t hdns_file_create_dir(const char *dir_path) {
    hdns_pool_new(mp);
    apr_finfo_t finfo;
    apr_status_t rv = apr_stat(&finfo, dir_path, APR_FINFO_TYPE, mp);
    if (APR_STATUS_IS_ENOENT(rv)) {
        rv = apr_dir_make(dir_path, APR_OS_DEFAULT, mp);
        hdns_log_info((rv == APR_SUCCESS ?
                       "Directory '%s' created successfully." : "Failed to create directory '%s'"), dir_path);

    } else if (rv == APR_SUCCESS && finfo.filetype == APR_DIR) {
        hdns_log_info("Directory '%s' already exists.", dir_path);
    } else {
        char errbuf[128];
        apr_strerror(rv, errbuf, sizeof(errbuf));
        hdns_log_info("Error checking directory '%s': %s", dir_path, errbuf);
    }
    hdns_pool_destroy(mp);
    return rv == APR_SUCCESS ? HDNS_OK : HDNS_ERROR;
}

int32_t hdns_file_write(const char *file_path, const char *content) {
    apr_file_t *file;
    apr_status_t rv;
    hdns_pool_new(mp);
    rv = apr_file_open(&file, file_path, APR_CREATE | APR_TRUNCATE | APR_WRITE, APR_OS_DEFAULT, mp);
    if (rv == APR_SUCCESS) {
        apr_size_t len = strlen(content);
        apr_size_t written = 0;
        while (len > written) {
            apr_size_t bytes_written = len - written;
            rv = apr_file_write(file, content + written, &bytes_written);
            if (rv != APR_SUCCESS) {
                char err_msg[128];
                apr_strerror(rv, err_msg, sizeof(err_msg));
                hdns_log_info("Error write content '%s' into file %s : %s", content, file_path, err_msg);
                break;
            }
            written += bytes_written;
        }
        apr_file_close(file);
    } else {
        char err_msg[128];
        apr_strerror(rv, err_msg, sizeof(err_msg));
        hdns_log_info("Error write content '%s' into file %s : %s", content, file_path, err_msg);
    }
    apr_pool_destroy(mp);
    return (rv == APR_SUCCESS) ? HDNS_OK : HDNS_ERROR;
}

char *hdns_file_read(const char *file_path, apr_pool_t *pool) {
    apr_file_t *file;
    apr_status_t rv;
    char *content = NULL;
    hdns_pool_new(mp);
    rv = apr_file_open(&file, file_path, APR_READ | APR_BUFFERED, APR_OS_DEFAULT, mp);
    if (rv == APR_SUCCESS) {
        char tmp_buf[512];
        hdns_list_head_t *content_buf = hdns_list_new(mp);
        apr_size_t bytes_read;
        while ((rv = apr_file_read_full(file, tmp_buf, sizeof(tmp_buf) - 1, &bytes_read)) == APR_SUCCESS ||
               rv == APR_EOF) {
            tmp_buf[bytes_read] = '\0';
            hdns_buf_t *b = hdns_create_buf(mp, bytes_read);
            memcpy(b->pos, tmp_buf, bytes_read);
            b->last += bytes_read;
            hdns_list_add(content_buf, b, NULL);
            if (rv == APR_EOF) break;
        }
        content = hdns_buf_list_content(pool, content_buf);
        apr_file_close(file);
    } else {
        char err_msg[128];
        apr_strerror(rv, err_msg, sizeof(err_msg));
        hdns_log_info("Error read file '%s': %s", file_path, err_msg);
    }
    apr_pool_destroy(mp);
    return content;
}