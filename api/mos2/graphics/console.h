#ifndef __graphics_console_h__
#define __graphics_console_h__

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

#include <stdint.h>

inline static int graphics_con_x(void) { // 195
    typedef int (*fn_ptr_t)(void);
    return ((fn_ptr_t)_sys_table_ptrs[195])();
}
inline static int graphics_con_y(void) { // 196
    typedef int (*fn_ptr_t)(void);
    return ((fn_ptr_t)_sys_table_ptrs[196])();
}
inline static void graphics_set_con_pos(int x, int y) {
    typedef void (*graphics_set_con_pos_ptr_t)(int x, int y);
    ((graphics_set_con_pos_ptr_t)_sys_table_ptrs[42])(x, y);
}
inline static void graphics_set_con_color(uint8_t color, uint8_t bgcolor) {
    typedef void (*graphics_set_con_color_ptr_t)(uint8_t color, uint8_t bgcolor);
    ((graphics_set_con_color_ptr_t)_sys_table_ptrs[43])(color, bgcolor);
}
inline static uint32_t get_console_width() {
    typedef uint32_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[146])();
}
inline static uint32_t get_console_height() {
    typedef uint32_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[147])();
}
inline static uint32_t get_screen_width() {
    typedef uint32_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[158])();
}
inline static uint32_t get_screen_height() {
    typedef uint32_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[159])();
}

inline static void set_cursor_color(uint8_t c) {
    typedef void (*fn_ptr_t)(uint8_t);
    ((fn_ptr_t)_sys_table_ptrs[186])(c);
}

#endif // __graphics_console_h__
