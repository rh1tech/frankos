#ifndef __graphics_draw_h__
#define __graphics_draw_h__

#include <graphics/lines.h>

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

inline static 
void draw_box(color_schema_t* pcs, int left, int top, int width, int height, const char* title, const lines_t* plines) {
    typedef void (*fn_ptr_t)(color_schema_t* pcs, int left, int top, int width, int height, const char* title, const lines_t* plines);
    ((fn_ptr_t)_sys_table_ptrs[240])(pcs, left, top, width, height, title, plines);
}

inline static 
void draw_panel(color_schema_t* pcs, int left, int top, int width, int height, const char* title, char* bottom) {
    typedef void (*fn_ptr_t)(color_schema_t* pcs, int left, int top, int width, int height, const char* title, char* bottom);
    ((fn_ptr_t)_sys_table_ptrs[241])(pcs, left, top, width, height, title, bottom);
}

inline static 
void draw_button(color_schema_t* pcs, int left, int top, int width, const char* txt, bool selected) {
    typedef void (*fn_ptr_t)(color_schema_t* pcs, int left, int top, int width, const char* txt, bool selected);
    ((fn_ptr_t)_sys_table_ptrs[242])(pcs, left, top, width, txt, selected);
}

inline static
void draw_text(const char *string, uint32_t x, uint32_t y, uint8_t color, uint8_t bgcolor) {
    #define _draw_text_ptr_idx 30
    typedef void (*draw_text_ptr_t)(const char *string, uint32_t x, uint32_t y, uint8_t color, uint8_t bgcolor);
    ((draw_text_ptr_t)_sys_table_ptrs[_draw_text_ptr_idx])(string, x, y, color, bgcolor);
}

inline static
void draw_window(const char* t, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    #define _draw_window_ptr_idx 31
    typedef void (*draw_window_ptr_t)(const char*, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    ((draw_window_ptr_t)_sys_table_ptrs[_draw_window_ptr_idx])(t, x, y, width, height);
}

inline static 
void draw_label(color_schema_t* pcs, int left, int top, int width, char* txt, bool selected, bool highlighted) {
    typedef void (*fn_ptr_t)(color_schema_t* pcs, int left, int top, int width, char* txt, bool selected, bool highlighted);
    ((fn_ptr_t)_sys_table_ptrs[239])(pcs, left, top, width, txt, selected, highlighted);
}

inline static
void show_logo(bool with_top) {
    typedef void (*fn_ptr_t)(bool);
    ((fn_ptr_t)_sys_table_ptrs[183])(with_top);
}

#endif // __graphics_draw_h__
