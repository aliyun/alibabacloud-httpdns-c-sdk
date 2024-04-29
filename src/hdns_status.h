#ifndef HDNS_C_SDK_HDNS_STATUS_H
#define HDNS_C_SDK_HDNS_STATUS_H

#include "hdns_define.h"

HDNS_CPP_START

#define HDNS_STATUS_ERR_CODE_SIZE 32
#define HDNS_STATUS_ERR_MSG_SIZE  256
#define HDNS_STATUS_SID_SIZE      (HDNS_SID_STRING_LEN + 1)


typedef struct hdns_status_s {
    int code;
    char error_code[HDNS_STATUS_ERR_CODE_SIZE];
    char error_msg[HDNS_STATUS_ERR_MSG_SIZE];
    char session_id[HDNS_STATUS_SID_SIZE];
} hdns_status_t;

int hdns_status_is_ok(hdns_status_t *s);

hdns_status_t hdns_status_ok(char *session_id);

hdns_status_t hdns_status_error(int code,
                                const char *error_code,
                                const char *error_msg,
                                const char *session_id);

extern const char HDNS_INVALID_ARGUMENT_CODE[];
extern const char HDNS_FAILED_VERIFICATION_CODE[];
extern const char HDNS_RESOLVE_FAIL_CODE[];
extern const char HDNS_OUT_MEMORY_CODE[];
extern const char HDNS_SCHEDULE_FAIL_CODE[];
extern const char HDNS_OPEN_FILE_ERROR_CODE[];

HDNS_CPP_END

#endif
