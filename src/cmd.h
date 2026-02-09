#ifndef CMD_H
#define CMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include "ff.h"
#include "FreeRTOS.h"
#include "task.h"
#include "../api/m-os-api-c-list.h"
#include "../api/m-os-api-c-array.h"

/* Forward declaration — terminal_t is defined in terminal.h */
#ifndef TERMINAL_T_DEFINED
#define TERMINAL_T_DEFINED
typedef struct terminal terminal_t;
#endif

size_t get_heap_total();

char* get_curr_dir(); // old system
FIL * get_stdout(); // old system
FIL * get_stderr(); // old system

extern array_t* /*of cmd_ctx_t*/ pids;

typedef struct {
    char* del_addr;
    char* prg_addr;
    int sec_num;
} sect_entry_t;

static void sect_entry_deallocator(sect_entry_t* s) {
    if (s->del_addr) vPortFree(s->del_addr);
    vPortFree(s);
}

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
    list_t* /*sect_entry_t*/ sections;
    void* _fini_ctx;
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
    SIGTERM,
    ZOMBIE
} cmd_exec_stage_t;

#define MAX_SIG 32 

typedef void (*sighandler_t)(int);

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
    TaskHandle_t task;
    TaskHandle_t parent_task; // TODO: optimise (ppid only?)
    long pid;
    long ppid;
    long pgid;
    long sid;
    long gid;
    long egid;
    long uid;
    long euid;
    long ctty;
    uint32_t sig_pending;        // битовая маска сигналов
    uint32_t sig_blocked;        // маска заблокированных сигналов
    sighandler_t sig_handler[MAX_SIG];
    uint32_t sig_default;        // маска сигналов с дефолтным действием
    array_t /*of FDESC*/ *pfiles; // open files per process
    array_t /*of DIR*/ *pdirs; // open directories per process
    list_t /*of void*/ *pallocs; // related to the process allocations
    int proc_errno;
    terminal_t *term;   /* owning terminal for this process */
} cmd_ctx_t;

cmd_ctx_t* get_cmd_startup_ctx(); // system
cmd_ctx_t* get_cmd_ctx();
void set_cmd_ctx(cmd_ctx_t* ctx);
cmd_ctx_t* clone_ctx(cmd_ctx_t* src);
void remove_ctx(cmd_ctx_t*);
void set_ctx_var(cmd_ctx_t*, const char* key, const char* val);
char* get_ctx_var(cmd_ctx_t*, const char* key);
void cleanup_ctx(cmd_ctx_t* src);

char* next_token(char* t);
char* concat(const char* s1, const char* s2);
bool exists(cmd_ctx_t* ctx);
char* concat2(const char* s1, size_t sz, const char* s2);
char* copy_str(const char* s);
void show_logo(bool with_top);

// helpers for `cmd` and `mc` to reduce their size
#include "FreeRTOS.h"
#include "task.h"
#include "../api/m-os-api-c-string.h"

void cmd_tab(cmd_ctx_t* ctx, string_t* s_cmd);
int history_steps(cmd_ctx_t* ctx, int cmd_history_idx, string_t* s_cmd);
bool cmd_enter_helper(cmd_ctx_t* ctx, string_t* s_cmd);

typedef FRESULT (*FRFpvUpU_ptr_t)(FIL*, void*, UINT, UINT*);
void op_console(cmd_ctx_t* ctx, FRFpvUpU_ptr_t fn, BYTE mode);
bool f_read_str(FIL* f, char* buf, size_t lim);

#ifdef __cplusplus
}
#endif

#endif // CMD_H
