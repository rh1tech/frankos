#ifndef M_API_C_SORT
#define M_API_C_SORT

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

#ifndef __compar_fn_t_defined
#define __compar_fn_t_defined
typedef int (*__compar_fn_t) (const void *, const void *);
#endif

inline static
void qsort(void *__base, size_t __nmemb, size_t __size, __compar_fn_t _compar) {
    typedef void (*fn_ptr_t)(void *__base, size_t __nmemb, size_t __size, __compar_fn_t _compar);
    ((fn_ptr_t)_sys_table_ptrs[169])(__base, __nmemb, __size, _compar);
}


#ifdef __cplusplus
}
#endif

#endif
