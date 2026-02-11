/*
 * FRANK OS Application API
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * Standalone ELF apps include this header to access FRANK OS GUI services
 * through the MOS2 sys_table at 0x10FFF000.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FRANKOS_APP_H
#define FRANKOS_APP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * sys_table base (inherited from m-os-api.h)
 * ======================================================================== */

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs =
    (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

/* ========================================================================
 * Geometry types
 * ======================================================================== */

typedef struct { int16_t x, y, w, h; } rect_t;
typedef struct { int16_t x, y; } point_t;

/* ========================================================================
 * Color constants (CGA/EGA 16-color palette)
 * ======================================================================== */

#define COLOR_BLACK          0
#define COLOR_BLUE           1
#define COLOR_GREEN          2
#define COLOR_CYAN           3
#define COLOR_RED            4
#define COLOR_MAGENTA        5
#define COLOR_BROWN          6
#define COLOR_LIGHT_GRAY     7
#define COLOR_DARK_GRAY      8
#define COLOR_LIGHT_BLUE     9
#define COLOR_LIGHT_GREEN   10
#define COLOR_LIGHT_CYAN    11
#define COLOR_LIGHT_RED     12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_YELLOW        14
#define COLOR_WHITE         15

/* ========================================================================
 * Window handle
 * ======================================================================== */

typedef uint8_t hwnd_t;
#define HWND_NULL    0
#define WM_MAX_WINDOWS 16

/* ========================================================================
 * Window flags
 * ======================================================================== */

#define WF_ALIVE     (1u << 0)
#define WF_VISIBLE   (1u << 1)
#define WF_FOCUSED   (1u << 2)
#define WF_CLOSABLE  (1u << 3)
#define WF_RESIZABLE (1u << 4)
#define WF_MOVABLE   (1u << 5)
#define WF_BORDER    (1u << 6)
#define WF_DIRTY     (1u << 7)
#define WF_MENUBAR   (1u << 8)

/* ========================================================================
 * Window style presets
 * ======================================================================== */

#define WSTYLE_DEFAULT   (WF_CLOSABLE | WF_RESIZABLE | WF_MOVABLE | WF_BORDER)
#define WSTYLE_DIALOG    (WF_CLOSABLE | WF_MOVABLE | WF_BORDER)
#define WSTYLE_POPUP     0

/* ========================================================================
 * Window event types (WM_* message IDs)
 * ======================================================================== */

#define WM_NULL          0
#define WM_CREATE        1
#define WM_DESTROY       2
#define WM_CLOSE         3
#define WM_PAINT         4
#define WM_SETFOCUS      5
#define WM_KILLFOCUS     6
#define WM_MOVE          7
#define WM_SIZE          8
#define WM_MINIMIZE      9
#define WM_MAXIMIZE     10
#define WM_RESTORE      11
#define WM_KEYDOWN      12
#define WM_KEYUP        13
#define WM_CHAR         14
#define WM_MOUSEMOVE    15
#define WM_LBUTTONDOWN  16
#define WM_LBUTTONUP    17
#define WM_RBUTTONDOWN  18
#define WM_RBUTTONUP    19
#define WM_TIMER        20
#define WM_COMMAND      21

/* Keyboard modifier flags */
#define KMOD_SHIFT  (1u << 0)
#define KMOD_CTRL   (1u << 1)
#define KMOD_ALT    (1u << 2)

/* ========================================================================
 * Window event structure
 * ======================================================================== */

typedef struct window_event {
    uint8_t type;
    uint8_t _pad;
    union {
        struct { uint8_t scancode; uint8_t modifiers; } key;
        struct { char ch; uint8_t modifiers; } charev;
        struct { int16_t x; int16_t y; uint8_t buttons; uint8_t modifiers; } mouse;
        struct { int16_t x; int16_t y; } move;
        struct { int16_t w; int16_t h; } size;
        struct { uint16_t id; } command;
        struct { uint16_t timer_id; } timer;
    };
} window_event_t;

/* ========================================================================
 * Forward declarations and typedefs
 * ======================================================================== */

typedef struct window window_t;
typedef bool (*event_handler_t)(hwnd_t hwnd, const window_event_t *event);
typedef void (*paint_handler_t)(hwnd_t hwnd);

/* ========================================================================
 * Window structure (must match firmware layout exactly)
 * ======================================================================== */

struct window {
    uint16_t         flags;
    uint8_t          state;
    rect_t           frame;
    rect_t           restore_rect;
    uint8_t          bg_color;
    uint8_t          z_order;
    char             title[24];
    event_handler_t  event_handler;
    paint_handler_t  paint_handler;
    void            *user_data;
};

/* ========================================================================
 * Menu types
 * ======================================================================== */

#define MENU_MAX_ITEMS    8
#define MENU_MAX_MENUS    4
#define MIF_SEPARATOR    (1u << 0)
#define MIF_DISABLED     (1u << 1)

typedef struct {
    char     text[20];
    uint16_t command_id;
    uint8_t  flags;
    uint8_t  accel_key;
} menu_item_t;

typedef struct {
    char        title[12];
    uint8_t     accel_key;
    uint8_t     item_count;
    menu_item_t items[MENU_MAX_ITEMS];
} menu_def_t;

typedef struct {
    uint8_t    menu_count;
    menu_def_t menus[MENU_MAX_MENUS];
} menu_bar_t;

/* ========================================================================
 * Dialog constants
 * ======================================================================== */

#define DLG_ICON_NONE       0
#define DLG_ICON_INFO       1
#define DLG_ICON_WARNING    2
#define DLG_ICON_ERROR      3

#define DLG_BTN_OK          (1u << 0)
#define DLG_BTN_CANCEL      (1u << 1)
#define DLG_BTN_YES         (1u << 2)
#define DLG_BTN_NO          (1u << 3)

/* ========================================================================
 * Theme constants
 * ======================================================================== */

#define THEME_BUTTON_FACE    COLOR_LIGHT_GRAY
#define THEME_TITLE_HEIGHT   20
#define THEME_MENU_HEIGHT    20
#define THEME_BORDER_WIDTH    4

/* ========================================================================
 * Display / taskbar constants
 * ======================================================================== */

#define DISPLAY_WIDTH   640
#define DISPLAY_HEIGHT  480
#define TASKBAR_HEIGHT   28

/* ========================================================================
 * Font constants
 * ======================================================================== */

#define FONT_UI_WIDTH    6
#define FONT_UI_HEIGHT  12

/* ========================================================================
 * FreeRTOS timer types
 * ======================================================================== */

typedef void *TimerHandle_t;
typedef void (*TimerCallbackFunction_t)(TimerHandle_t xTimer);

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(xTimeInMs) ((uint32_t)((xTimeInMs) * configTICK_RATE_HZ / 1000))
#endif

#ifndef configTICK_RATE_HZ
#define configTICK_RATE_HZ 1000
#endif

#ifndef pdTRUE
#define pdTRUE  1
#endif

#ifndef pdFALSE
#define pdFALSE 0
#endif

#ifndef portMAX_DELAY
#define portMAX_DELAY 0xFFFFFFFFUL
#endif

/* ========================================================================
 * Inline sys_table wrappers — FRANK OS GUI API (indices 404–433)
 * ======================================================================== */

/* 404: wm_create_window */
static inline hwnd_t wm_create_window(int16_t x, int16_t y, int16_t w, int16_t h,
                                        const char *title, uint16_t style,
                                        event_handler_t event_cb,
                                        paint_handler_t paint_cb) {
    typedef hwnd_t (*fn_t)(int16_t, int16_t, int16_t, int16_t,
                           const char*, uint16_t, event_handler_t, paint_handler_t);
    return ((fn_t)_sys_table_ptrs[404])(x, y, w, h, title, style, event_cb, paint_cb);
}

/* 405: wm_destroy_window */
static inline void wm_destroy_window(hwnd_t hwnd) {
    typedef void (*fn_t)(hwnd_t);
    ((fn_t)_sys_table_ptrs[405])(hwnd);
}

/* 406: wm_show_window */
static inline void wm_show_window(hwnd_t hwnd) {
    typedef void (*fn_t)(hwnd_t);
    ((fn_t)_sys_table_ptrs[406])(hwnd);
}

/* 407: wm_set_focus */
static inline void wm_set_focus(hwnd_t hwnd) {
    typedef void (*fn_t)(hwnd_t);
    ((fn_t)_sys_table_ptrs[407])(hwnd);
}

/* 408: wm_get_window */
static inline window_t *wm_get_window(hwnd_t hwnd) {
    typedef window_t *(*fn_t)(hwnd_t);
    return ((fn_t)_sys_table_ptrs[408])(hwnd);
}

/* 409: wm_set_window_rect */
static inline void wm_set_window_rect(hwnd_t hwnd, int16_t x, int16_t y,
                                        int16_t w, int16_t h) {
    typedef void (*fn_t)(hwnd_t, int16_t, int16_t, int16_t, int16_t);
    ((fn_t)_sys_table_ptrs[409])(hwnd, x, y, w, h);
}

/* 410: wm_invalidate */
static inline void wm_invalidate(hwnd_t hwnd) {
    typedef void (*fn_t)(hwnd_t);
    ((fn_t)_sys_table_ptrs[410])(hwnd);
}

/* 411: wm_post_event */
static inline bool wm_post_event(hwnd_t hwnd, const window_event_t *event) {
    typedef bool (*fn_t)(hwnd_t, const window_event_t*);
    return ((fn_t)_sys_table_ptrs[411])(hwnd, event);
}

/* 412: wm_get_client_rect */
static inline rect_t wm_get_client_rect(hwnd_t hwnd) {
    typedef rect_t (*fn_t)(hwnd_t);
    return ((fn_t)_sys_table_ptrs[412])(hwnd);
}

/* 413: wd_begin */
static inline void wd_begin(hwnd_t hwnd) {
    typedef void (*fn_t)(hwnd_t);
    ((fn_t)_sys_table_ptrs[413])(hwnd);
}

/* 414: wd_end */
static inline void wd_end(void) {
    typedef void (*fn_t)(void);
    ((fn_t)_sys_table_ptrs[414])();
}

/* 415: wd_pixel */
static inline void wd_pixel(int16_t x, int16_t y, uint8_t color) {
    typedef void (*fn_t)(int16_t, int16_t, uint8_t);
    ((fn_t)_sys_table_ptrs[415])(x, y, color);
}

/* 416: wd_hline */
static inline void wd_hline(int16_t x, int16_t y, int16_t w, uint8_t color) {
    typedef void (*fn_t)(int16_t, int16_t, int16_t, uint8_t);
    ((fn_t)_sys_table_ptrs[416])(x, y, w, color);
}

/* 417: wd_vline */
static inline void wd_vline(int16_t x, int16_t y, int16_t h, uint8_t color) {
    typedef void (*fn_t)(int16_t, int16_t, int16_t, uint8_t);
    ((fn_t)_sys_table_ptrs[417])(x, y, h, color);
}

/* 418: wd_fill_rect */
static inline void wd_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                                  uint8_t color) {
    typedef void (*fn_t)(int16_t, int16_t, int16_t, int16_t, uint8_t);
    ((fn_t)_sys_table_ptrs[418])(x, y, w, h, color);
}

/* 419: wd_clear */
static inline void wd_clear(uint8_t color) {
    typedef void (*fn_t)(uint8_t);
    ((fn_t)_sys_table_ptrs[419])(color);
}

/* 420: wd_rect */
static inline void wd_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint8_t color) {
    typedef void (*fn_t)(int16_t, int16_t, int16_t, int16_t, uint8_t);
    ((fn_t)_sys_table_ptrs[420])(x, y, w, h, color);
}

/* 421: wd_bevel_rect */
static inline void wd_bevel_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                                   uint8_t light, uint8_t dark, uint8_t face) {
    typedef void (*fn_t)(int16_t, int16_t, int16_t, int16_t,
                         uint8_t, uint8_t, uint8_t);
    ((fn_t)_sys_table_ptrs[421])(x, y, w, h, light, dark, face);
}

/* 422: wd_char_ui */
static inline void wd_char_ui(int16_t x, int16_t y, char c,
                                uint8_t fg, uint8_t bg) {
    typedef void (*fn_t)(int16_t, int16_t, char, uint8_t, uint8_t);
    ((fn_t)_sys_table_ptrs[422])(x, y, c, fg, bg);
}

/* 423: wd_text_ui */
static inline void wd_text_ui(int16_t x, int16_t y, const char *str,
                                uint8_t fg, uint8_t bg) {
    typedef void (*fn_t)(int16_t, int16_t, const char*, uint8_t, uint8_t);
    ((fn_t)_sys_table_ptrs[423])(x, y, str, fg, bg);
}

/* 424: menu_set */
static inline void menu_set(hwnd_t hwnd, const menu_bar_t *bar) {
    typedef void (*fn_t)(hwnd_t, const menu_bar_t*);
    ((fn_t)_sys_table_ptrs[424])(hwnd, bar);
}

/* 425: dialog_show */
static inline hwnd_t dialog_show(hwnd_t parent, const char *title,
                                   const char *text, uint8_t icon,
                                   uint8_t buttons) {
    typedef hwnd_t (*fn_t)(hwnd_t, const char*, const char*, uint8_t, uint8_t);
    return ((fn_t)_sys_table_ptrs[425])(parent, title, text, icon, buttons);
}

/* 426: taskbar_invalidate */
static inline void taskbar_invalidate(void) {
    typedef void (*fn_t)(void);
    ((fn_t)_sys_table_ptrs[426])();
}

/* 427: xTimerCreate */
static inline TimerHandle_t xTimerCreate(const char *name, uint32_t period,
                                           uint32_t autoReload, void *pvTimerID,
                                           TimerCallbackFunction_t callback) {
    typedef TimerHandle_t (*fn_t)(const char*, uint32_t, uint32_t, void*,
                                   TimerCallbackFunction_t);
    return ((fn_t)_sys_table_ptrs[427])(name, period, autoReload, pvTimerID, callback);
}

/* 428: xTimerGenericCommandFromTask — underlying function for timer macros.
 * FreeRTOS command IDs: START=1, STOP=3, DELETE=5 */
#define _tmrCOMMAND_START   1
#define _tmrCOMMAND_STOP    3
#define _tmrCOMMAND_DELETE  5

/* xTaskGetTickCount is at sys_table index 17 */
static inline uint32_t _app_xTaskGetTickCount(void) {
    typedef uint32_t (*fn_t)(void);
    return ((fn_t)_sys_table_ptrs[17])();
}
#define xTaskGetTickCount _app_xTaskGetTickCount

static inline int32_t xTimerStart(TimerHandle_t xTimer, uint32_t xTicksToWait) {
    typedef int32_t (*fn_t)(TimerHandle_t, int32_t, uint32_t, int32_t*, uint32_t);
    return ((fn_t)_sys_table_ptrs[428])(xTimer, _tmrCOMMAND_START,
                                         _app_xTaskGetTickCount(), 0, xTicksToWait);
}

static inline int32_t xTimerStop(TimerHandle_t xTimer, uint32_t xTicksToWait) {
    typedef int32_t (*fn_t)(TimerHandle_t, int32_t, uint32_t, int32_t*, uint32_t);
    return ((fn_t)_sys_table_ptrs[428])(xTimer, _tmrCOMMAND_STOP,
                                         0, 0, xTicksToWait);
}

static inline int32_t xTimerDelete(TimerHandle_t xTimer, uint32_t xTicksToWait) {
    typedef int32_t (*fn_t)(TimerHandle_t, int32_t, uint32_t, int32_t*, uint32_t);
    return ((fn_t)_sys_table_ptrs[428])(xTimer, _tmrCOMMAND_DELETE,
                                         0, 0, xTicksToWait);
}

/* 429: pvTimerGetTimerID */
static inline void *pvTimerGetTimerID(TimerHandle_t xTimer) {
    typedef void *(*fn_t)(TimerHandle_t);
    return ((fn_t)_sys_table_ptrs[429])(xTimer);
}

/* 430: xTaskGenericNotify — underlying function for xTaskNotifyGive.
 * eIncrement = 3 (eNotifyAction enum value) */
#define _eIncrement 3

static inline int32_t xTaskNotifyGive(void *xTaskToNotify) {
    typedef int32_t (*fn_t)(void*, uint32_t, uint32_t, int32_t, uint32_t*);
    return ((fn_t)_sys_table_ptrs[430])(xTaskToNotify, 0, 0, _eIncrement, 0);
}

/* 431: ulTaskGenericNotifyTake */
static inline uint32_t ulTaskNotifyTake(int32_t xClearCountOnExit,
                                          uint32_t xTicksToWait) {
    typedef uint32_t (*fn_t)(uint32_t, int32_t, uint32_t);
    return ((fn_t)_sys_table_ptrs[431])(0, xClearCountOnExit, xTicksToWait);
}

#ifdef __cplusplus
}
#endif

#endif /* FRANKOS_APP_H */
