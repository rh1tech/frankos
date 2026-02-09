#ifndef __system_usbd_h__
#define __system_usbd_h__

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

// USB
// (better use usb_driver)
inline static void init_pico_usb_drive() {
    typedef void (*fn_ptr_t)();
    ((fn_ptr_t)_sys_table_ptrs[179])();
}

// (better use usb_driver)
inline static void pico_usb_drive_heartbeat() {
    typedef void (*fn_ptr_t)();
    ((fn_ptr_t)_sys_table_ptrs[180])();
}

inline static bool tud_msc_ejected() {
    typedef bool (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[181])();
}

// (better use usb_driver)
inline static void set_tud_msc_ejected(bool v) {
    typedef void (*fn_ptr_t)(bool);
    ((fn_ptr_t)_sys_table_ptrs[182])(v);
}

inline static void usb_driver(bool on) {
    typedef void (*fn_ptr_t)(bool);
    ((fn_ptr_t)_sys_table_ptrs[185])(on);
}

typedef void (*usb_detached_handler_t)(void);
inline static bool set_usb_detached_handler(usb_detached_handler_t h) {
    typedef bool (*fn_ptr_t)(usb_detached_handler_t);
    return ((fn_ptr_t)_sys_table_ptrs[236])(h);
}

#endif // __system_usbd_h__
