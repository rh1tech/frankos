#include "errno.h"
#include "cmd.h"
#include "sys_table.h"

int* __in_hfa() __errno_location() {
    cmd_ctx_t* ctx = get_cmd_ctx();
    return &ctx->proc_errno;
}
