#ifndef PICO_TIMER
#define PICO_TIMER

#include <stdint.h>
#include "m-os-api.h"

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

#endif
