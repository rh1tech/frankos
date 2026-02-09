#ifndef __system_cmd_h__
#define __system_cmd_h__

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

#include <stdint.h>
#include <stdbool.h>
#include <system/ff.h>
#include <system/rtos.h>
#include <data/c-string.h>

typedef struct {
    char* del_addr;
    char* prg_addr;
    uint16_t sec_num;
} sect_entry_t;

typedef int (*bootb_req_ver_ptr_t)( void );
typedef void* (*bootb_init_ptr_t)( void );
typedef int (*bootb_main_ptr_t)( int, char** );
typedef void (*bootb_fini_ptr_t)( void* );
typedef void (*bootb_sig_ptr_t)( int );

typedef struct {
    bootb_req_ver_ptr_t req_ver_fn;
    bootb_init_ptr_t _init_fn;
    bootb_main_ptr_t main_fn;
    bootb_fini_ptr_t _fini_fn;
    bootb_sig_ptr_t sig_fn;
    void* /* list_ of sect_entry_t*/ sections;
} bootb_ctx_t;

typedef struct {
    const char* key;
    char* value;
} vars_t;

typedef enum {
    INITIAL,
    PREPARED,
    FOUND,
    VALID,
    LOAD,
    EXECUTED,
    INVALIDATED,
    SIGTERM
} cmd_exec_stage_t;

typedef struct cmd_ctx {
    uint32_t argc;
    char** argv;
    char* orig_cmd;

    FIL* std_in;
    FIL* std_out;
    FIL* std_err;
    int ret_code;
    bool detached;

    struct cmd_ctx* prev;
    bootb_ctx_t* pboot_ctx;

    vars_t* vars;
    size_t vars_num;

    struct cmd_ctx* next;

    volatile cmd_exec_stage_t stage;
    void* user_data;
    bool forse_flash;
    TaskHandle_t parent_task;
} cmd_ctx_t;

inline static cmd_ctx_t* get_cmd_ctx() {
    typedef cmd_ctx_t* (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[138])();
}
inline static void cleanup_ctx(cmd_ctx_t* src) {
    typedef void (*fn_ptr_t)(cmd_ctx_t*);
    ((fn_ptr_t)_sys_table_ptrs[139])(src);
}
inline static char* get_ctx_var(cmd_ctx_t* src, const char* k) {
    typedef char* (*fn_ptr_t)(cmd_ctx_t*, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[140])(src, k);
}
inline static cmd_ctx_t* set_ctx_var(cmd_ctx_t* src, const char* k, char* v) {
    typedef void (*fn_ptr_t)(cmd_ctx_t*, const char*, char*);
    ((fn_ptr_t)_sys_table_ptrs[141])(src, k, v);
}

inline static void cmd_tab(cmd_ctx_t* ctx, string_t* s_cmd) {
    typedef void (*fn_ptr_t)(cmd_ctx_t* ctx, string_t* s_cmd);
    ((fn_ptr_t)_sys_table_ptrs[233])(ctx, s_cmd);
}

inline static int history_steps(cmd_ctx_t* ctx, int cmd_history_idx, string_t* s_cmd) {
    typedef int (*fn_ptr_t)(cmd_ctx_t* ctx, int cmd_history_idx, string_t* s_cmd);
    return ((fn_ptr_t)_sys_table_ptrs[234])(ctx, cmd_history_idx, s_cmd);
}

inline static bool cmd_enter_helper(cmd_ctx_t* ctx, string_t* s_cmd) {
    typedef bool (*fn_ptr_t)(cmd_ctx_t* ctx, string_t* s_cmd);
    return ((fn_ptr_t)_sys_table_ptrs[235])(ctx, s_cmd);
}

#endif // __system_cmd_h__
