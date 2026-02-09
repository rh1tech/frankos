#ifndef __system_terminal_h__
#define __system_terminal_h__

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

#define CHAR_CODE_BS    8
#define CHAR_CODE_UP    17
#define CHAR_CODE_DOWN  18
#define CHAR_CODE_ENTER '\n'
#define CHAR_CODE_TAB   '\t'
#define CHAR_CODE_ESC   0x1B
#define CHAR_CODE_EOF   0xFF

#define PS2_LED_SCROLL_LOCK 1
#define PS2_LED_NUM_LOCK    2
#define PS2_LED_CAPS_LOCK   4

inline static uint8_t get_leds_stat() {
    typedef uint8_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[64])();
}

inline static char getch(void) {
    typedef char (*fn_ptr_t)(void);
    return ((fn_ptr_t)_sys_table_ptrs[122])();
}

inline static char getch_now(void) {
    typedef char (*fn_ptr_t)(void);
    return ((fn_ptr_t)_sys_table_ptrs[184])();
}

typedef bool (*scancode_handler_t)(const uint32_t);
inline static scancode_handler_t get_scancode_handler() {
    typedef scancode_handler_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[115])();
}
inline static void set_scancode_handler(scancode_handler_t h) {
    typedef void (*fn_ptr_t)(scancode_handler_t);
    ((fn_ptr_t)_sys_table_ptrs[116])(h);
}
typedef bool (*cp866_handler_t)(const char, uint32_t);
inline static cp866_handler_t get_cp866_handler() {
    typedef cp866_handler_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[117])();
}
inline static void set_cp866_handler(cp866_handler_t h) {
    typedef void (*fn_ptr_t)(cp866_handler_t);
    ((fn_ptr_t)_sys_table_ptrs[118])(h);
}
inline static void gbackspace() {
    typedef void (*fn_ptr_t)();
    ((fn_ptr_t)_sys_table_ptrs[119])();
}

#endif // __system_terminal_h__
