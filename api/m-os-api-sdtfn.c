// use it to resolve issues like memset and/or memcpy are not found on elf32 obj execution attempt
#include "m-os-api.h"

void* memset(void* p, int v, size_t sz) {
    typedef void* (*fn)(void *, int, size_t);
    return ((fn)_sys_table_ptrs[142])(p, v, sz);
}

void* memcpy(void *__restrict dst, const void *__restrict src, size_t sz) {
    typedef void* (*fn)(void *, const void*, size_t);
    return ((fn)_sys_table_ptrs[167])(dst, src, sz);
}

char* strcpy(char* t, const char * s) {
    typedef char* (*fn_ptr_t)(char*, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[60])(t, s);
}

void* memmove(void* dst, const void* src, size_t sz) {
    typedef void* (*fn)(void *, const void*, size_t);
    return ((fn)_sys_table_ptrs[232])(dst, src, sz);
}

char* strcat(char* t, const char * s) {
    typedef char* (*fn_ptr_t)(char*, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[252])(t, s);
}

#ifndef UF2_MODE
void _exit(int status) { goutf("_exit(%d)\n", status); }
void _kill(void) { gouta("_kill\n"); }
int _getpid(void) { gouta("_getpid\n"); return 0; }
void _sbrk(void) { gouta("_sbrk\n"); }
void _write(void) { gouta("_write\n"); }
void _close(void) { gouta("_close\n"); }
void _fstat(void) { gouta("_fstat\n"); }
void _isatty(void) { gouta("_isatty\n"); }
void _lseek(void) { gouta("_lseek\n"); }
void _read(void) { gouta("_read\n"); }
void __cxa_pure_virtual(void) { gouta("__cxa_pure_virtual\n"); }
void __cxa_end_cleanup(void) { gouta("__cxa_end_cleanup\n"); }
void __aeabi_atexit(void) { gouta("__aeabi_atexit\n"); }
void __dso_handle(void) { gouta("__dso_handle\n"); }

#ifndef marked_to_exit
volatile bool marked_to_exit;

int __required_m_api_verion(void) {
    return M_API_VERSION;
}

// only SIGTERM is supported for now
int signal(void) {
    marked_to_exit = true;
    return 0;
}
#endif

#endif
/*
2512 00000000h NOTYPE  GLOBAL __aeabi_unwind_cpp_pr0 (0) -> 0 UNDEF
2605 00000000h NOTYPE  GLOBAL __gnu_thumb1_case_uqi (0) -> 0 UNDEF
2621 00000000h NOTYPE  GLOBAL __aeabi_d2uiz (0) -> 0 UNDEF
2631 00000000h NOTYPE  GLOBAL __gxx_personality_v0 (0) -> 0 UNDEF
2732 00000000h NOTYPE  GLOBAL __gnu_thumb1_case_uhi (0) -> 0 UNDEF
2764 00000000h NOTYPE  GLOBAL __gnu_thumb1_case_sqi (0) -> 0 UNDEF
*/
