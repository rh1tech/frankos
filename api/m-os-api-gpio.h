#ifndef PICO_GPIO
#define PICO_GPIO

#define LED_BUILTIN 25

#include "m-os-api-timer.h"

static inline void gpio_put(unsigned gpio, bool value) {
    typedef void (*fn_ptr_t)(unsigned, bool);
    ((fn_ptr_t)_sys_table_ptrs[260])(gpio, value);
}

#endif
