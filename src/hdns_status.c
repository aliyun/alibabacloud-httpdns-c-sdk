#include "hdns_log.h"
#include "hdns_status.h"


const char HDNS_INVALID_ARGUMENT_CODE[] = "InvalidArgument";
const char HDNS_FAILED_VERIFICATION_CODE[] = "FailedVerification";
const char HDNS_RESOLVE_FAIL_CODE[] = "ResolveFail";
const char HDNS_OUT_MEMORY_CODE[] = "OutMemoryError";
const char HDNS_SCHEDULE_FAIL_CODE[] = "ScheduleFail";
const char HDNS_OPEN_FILE_ERROR_CODE[] = "OpenFileFail";


hdns_status_t hdns_status_ok(char* session_id) {
    hdns_status_t status = {
            .code = HDNS_OK,
            .error_code = "",
            .error_msg = "",
            .session_id = ""
    };
    if (session_id != NULL) {
        strncpy(status.session_id, session_id, HDNS_STATUS_SID_SIZE);
        status.session_id[HDNS_STATUS_SID_SIZE - 1] = '\0';
    }
    return status;
}

hdns_status_t hdns_status_error(int code,
                                const char *error_code,
                                const char *error_msg,
                                const char *session_id) {
    hdns_status_t status = hdns_status_ok(NULL);
    status.code = code;
    if (error_code != NULL) {
        strncpy(status.error_code, error_code, HDNS_STATUS_ERR_CODE_SIZE);
        status.error_code[HDNS_STATUS_ERR_CODE_SIZE - 1] = '\0';
    }
    if (error_msg != NULL) {
        strncpy(status.error_msg, error_msg, HDNS_STATUS_ERR_MSG_SIZE);
        status.error_msg[HDNS_STATUS_ERR_MSG_SIZE - 1] = '\0';
    }
    if (session_id != NULL) {
        strncpy(status.session_id, session_id, HDNS_STATUS_SID_SIZE);
        status.session_id[HDNS_STATUS_SID_SIZE - 1] = '\0';
    }
    return status;
}

int hdns_status_is_ok(hdns_status_t *s) {
    return s != NULL && s->code == HDNS_OK;
}
