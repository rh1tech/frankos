#ifndef __system_nespad_h__
#define __system_nespad_h__

#include <stdint.h>

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

#define DPAD_LEFT 0x40
#define DPAD_RIGHT 0x80
#define DPAD_DOWN 0x20
#define DPAD_UP 0x10
#define DPAD_START 0x08
#define DPAD_SELECT 0x04
#define DPAD_B 0x02
#define DPAD_A 0x01

inline static void nespad_stat(uint8_t* pad1, uint8_t* pad2) {
    typedef void (*fn_ptr_t)(uint8_t*, uint8_t*);
    ((fn_ptr_t)_sys_table_ptrs[187])(pad1, pad2);
}

#endif // __system_nespad_h__
