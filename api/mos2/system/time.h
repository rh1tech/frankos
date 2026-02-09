#ifndef __system_time_h__
#define __system_time_h__

#include <stdint.h>

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

//#define _xTaskDelayUntilPtrIdx 3
//#define _xTaskAbortDelayPtrIdx 4

inline static void sleep_ms( const uint32_t xTicksToDelay ) {
    #define _vTaskDelayPtrIdx 2
    typedef void (*vTaskDelay_ptr_t)( const uint32_t xTicksToDelay );
    ((vTaskDelay_ptr_t)_sys_table_ptrs[_vTaskDelayPtrIdx])( xTicksToDelay );
}

static inline uint64_t time_us_64(void) {
    typedef uint64_t (*fn_ptr_t)(void);
    return ((fn_ptr_t)_sys_table_ptrs[263])();
}
static inline unsigned time_us_32(void) {
    typedef unsigned (*fn_ptr_t)(void);
    return ((fn_ptr_t)_sys_table_ptrs[262])();
}
static inline unsigned time(unsigned x) {
    typedef unsigned (*fn_ptr_t)(unsigned);
    return ((fn_ptr_t)_sys_table_ptrs[261])(x);
}

#endif // __system_time_h__
