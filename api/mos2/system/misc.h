#ifndef __system_misc_h__
#define __system_misc_h__

#include <system/ff.h>

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

typedef FRESULT (*FRFpvUpU_ptr_t)(FIL*, void*, UINT, UINT*);
inline static void op_console(cmd_ctx_t* ctx, FRFpvUpU_ptr_t fn, BYTE mode) {
    typedef void (*fn_ptr_t)(cmd_ctx_t* ctx, FRFpvUpU_ptr_t fn, BYTE mode);
    ((fn_ptr_t)_sys_table_ptrs[237])(ctx, fn, mode);
}

inline static void blimp(uint32_t cnt, uint32_t ticks_to_delay) { // 194
    typedef void (*fn_ptr_t)(uint32_t, uint32_t);
    ((fn_ptr_t)_sys_table_ptrs[194])(cnt, ticks_to_delay);
}

inline static void reboot_me( void ) {
    typedef void (*fn_ptr_t)(void);
    ((fn_ptr_t)_sys_table_ptrs[254])();
}

#endif // __system_misc_h__
