#ifndef _LIBGEN_H
#define _LIBGEN_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

inline static char* dirname(char* p) {
    typedef char* (*fn_ptr_t)(char*);
    return ((fn_ptr_t)_sys_table_ptrs[351])(p);
}
inline static char* basename(char* p) {
    typedef char* (*fn_ptr_t)(char*);
    return ((fn_ptr_t)_sys_table_ptrs[352])(p);
}

#ifdef __cplusplus
}
#endif

#endif
