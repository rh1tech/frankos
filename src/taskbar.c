/*
 * FRANK OS
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "taskbar.h"
#include "window.h"
#include "window_event.h"
#include "window_theme.h"
#include "startmenu.h"
#include "swap.h"
#include "gfx.h"
#include "font.h"
#include "display.h"
#include <string.h>

extern const uint8_t default_icon_16x16[256];

/*==========================================================================
 * Constants
 *=========================================================================*/

#define BUTTON_HEIGHT     22   /* height of taskbar buttons */
#define BUTTON_Y          (TASKBAR_Y + 3) /* y of taskbar buttons */
#define TASK_BUTTON_W    120   /* max width of per-window buttons */
#define TASK_BUTTON_PAD    2   /* padding between task buttons */
#define TASK_AREA_X       (TASKBAR_START_W + 6) /* where task buttons start */
#define TASK_AREA_W       (DISPLAY_WIDTH - TASK_AREA_X - 4)

/*==========================================================================
 * Internal state
 *=========================================================================*/

static bool taskbar_ready = false;
static bool taskbar_dirty = true;  /* starts dirty for first frame */

/* Compute the button width based on how many task buttons are visible */
static int taskbar_btn_width(void) {
    int count = 0;
    for (uint8_t i = 1; i <= WM_MAX_WINDOWS; i++) {
        window_t *w = wm_get_window(i);
        if (w && (w->flags & WF_ALIVE) && (w->flags & WF_BORDER)) count++;
    }
    if (count == 0) return TASK_BUTTON_W;
    int avail = TASK_AREA_W - (count - 1) * TASK_BUTTON_PAD;
    int w = avail / count;
    if (w > TASK_BUTTON_W) w = TASK_BUTTON_W;
    if (w < 30) w = 30;
    return w;
}

/*==========================================================================
 * Helpers
 *=========================================================================*/

/* Draw a Win95-style raised button */
static void draw_raised_button(int x, int y, int w, int h) {
    gfx_fill_rect(x, y, w, h, THEME_BUTTON_FACE);
    /* Outer highlight */
    gfx_hline(x, y, w, COLOR_WHITE);
    gfx_vline(x, y, h, COLOR_WHITE);
    gfx_hline(x, y + h - 1, w, COLOR_BLACK);
    gfx_vline(x + w - 1, y, h, COLOR_BLACK);
    /* Inner highlight */
    gfx_hline(x + 1, y + h - 2, w - 2, COLOR_DARK_GRAY);
    gfx_vline(x + w - 2, y + 1, h - 2, COLOR_DARK_GRAY);
}

/* Draw a Win95-style sunken button (for focused window's task button) */
static void draw_sunken_button(int x, int y, int w, int h) {
    gfx_fill_rect(x, y, w, h, THEME_BUTTON_FACE);
    gfx_hline(x, y, w, COLOR_DARK_GRAY);
    gfx_vline(x, y, h, COLOR_DARK_GRAY);
    gfx_hline(x, y + h - 1, w, COLOR_WHITE);
    gfx_vline(x + w - 1, y, h, COLOR_WHITE);
}

/*==========================================================================
 * Public API
 *=========================================================================*/

void taskbar_init(void) {
    taskbar_ready = true;
}

int16_t taskbar_work_area_height(void) {
    return DISPLAY_HEIGHT - TASKBAR_HEIGHT;
}

void taskbar_invalidate(void) {
    taskbar_dirty = true;
    wm_mark_dirty();
}

void taskbar_force_dirty(void) {
    taskbar_dirty = true;
}

bool taskbar_needs_redraw(void) {
    return taskbar_dirty;
}

void taskbar_draw(void) {
    if (!taskbar_ready || !taskbar_dirty) return;

    /* Suppress taskbar when focused window covers entire screen (fullscreen) */
    hwnd_t fs_focus = wm_get_focus();
    if (fs_focus != HWND_NULL) {
        window_t *fw = wm_get_window(fs_focus);
        if (fw && fw->frame.x == 0 && fw->frame.y == 0 &&
            fw->frame.w == DISPLAY_WIDTH && fw->frame.h == DISPLAY_HEIGHT &&
            !(fw->flags & WF_BORDER))
            return;
    }

    taskbar_dirty = false;

    int y = TASKBAR_Y;

    /* Main bar background */
    gfx_fill_rect(0, y, DISPLAY_WIDTH, TASKBAR_HEIGHT, THEME_BUTTON_FACE);

    /* Top raised edge */
    gfx_hline(0, y, DISPLAY_WIDTH, COLOR_WHITE);
    gfx_hline(0, y + 1, DISPLAY_WIDTH, THEME_BUTTON_FACE);

    /* Start button */
    bool start_open = startmenu_is_open();
    int start_x = 2;
    if (start_open) {
        draw_sunken_button(start_x, BUTTON_Y, TASKBAR_START_W, BUTTON_HEIGHT);
        gfx_text_ui_bold(start_x + 12 + 1, BUTTON_Y + (BUTTON_HEIGHT - FONT_UI_HEIGHT) / 2 + 1,
                         "Start", COLOR_BLACK, THEME_BUTTON_FACE);
    } else {
        draw_raised_button(start_x, BUTTON_Y, TASKBAR_START_W, BUTTON_HEIGHT);
        gfx_text_ui_bold(start_x + 12, BUTTON_Y + (BUTTON_HEIGHT - FONT_UI_HEIGHT) / 2,
                         "Start", COLOR_BLACK, THEME_BUTTON_FACE);
    }

    /* Per-window task buttons */
    hwnd_t focus = wm_get_focus();
    int btn_x = TASK_AREA_X;
    int btn_w = taskbar_btn_width();

    for (uint8_t i = 1; i <= WM_MAX_WINDOWS; i++) {
        window_t *win = wm_get_window(i);
        if (!win || !(win->flags & WF_ALIVE)) continue;
        if (!(win->flags & WF_BORDER)) continue; /* skip borderless popups */

        if (btn_x + btn_w > DISPLAY_WIDTH) break; /* no room for more buttons */

        bool is_focused = (i == focus) && (win->flags & WF_VISIBLE);

        if (is_focused) {
            draw_sunken_button(btn_x, BUTTON_Y, btn_w, BUTTON_HEIGHT);
        } else {
            draw_raised_button(btn_x, BUTTON_Y, btn_w, BUTTON_HEIGHT);
        }

        /* Draw 16x16 icon in button */
        int offset = is_focused ? 1 : 0;
        const uint8_t *icon = win->icon ? win->icon : default_icon_16x16;
        gfx_draw_icon_16(btn_x + 4 + offset, BUTTON_Y + 3 + offset, icon);

        /* Truncated title text (shifted right for icon).
         * Suspended apps show title in dark gray to indicate they're inactive. */
        int text_x = btn_x + 22 + offset;
        int text_y = BUTTON_Y + (BUTTON_HEIGHT - FONT_UI_HEIGHT) / 2 + offset;
        int text_max_w = btn_w - 26;
        if (text_max_w > 0) {
            uint8_t text_color = (win->flags & WF_SUSPENDED) ?
                                  COLOR_DARK_GRAY : COLOR_BLACK;
            gfx_text_ui_clipped(text_x, text_y, win->title,
                                text_color, THEME_BUTTON_FACE,
                                btn_x + 2, BUTTON_Y, btn_w - 4, BUTTON_HEIGHT);
        }

        btn_x += btn_w + TASK_BUTTON_PAD;
    }
}

bool taskbar_mouse_click(int16_t x, int16_t y) {
    if (y < TASKBAR_Y) return false;

    /* Start button */
    if (x >= 2 && x < 2 + TASKBAR_START_W &&
        y >= BUTTON_Y && y < BUTTON_Y + BUTTON_HEIGHT) {
        startmenu_toggle();
        return true;
    }

    /* Per-window task buttons */
    int btn_x = TASK_AREA_X;
    int btn_w = taskbar_btn_width();

    for (uint8_t i = 1; i <= WM_MAX_WINDOWS; i++) {
        window_t *win = wm_get_window(i);
        if (!win || !(win->flags & WF_ALIVE)) continue;
        if (!(win->flags & WF_BORDER)) continue;

        if (btn_x + btn_w > DISPLAY_WIDTH) break;

        if (x >= btn_x && x < btn_x + btn_w &&
            y >= BUTTON_Y && y < BUTTON_Y + BUTTON_HEIGHT) {
            hwnd_t hwnd = (hwnd_t)i;
            if (win->state == WS_MINIMIZED) {
                wm_restore_window(hwnd);
            }
            swap_switch_to(hwnd);
            wm_set_focus(hwnd);
            wm_mark_dirty();
            return true;
        }

        btn_x += btn_w + TASK_BUTTON_PAD;
    }

    return true; /* consumed the click (was in taskbar area) */
}

/*==========================================================================
 * Taskbar right-click context menu (standalone popup, like sysmenu)
 *=========================================================================*/

#define TB_ITEM_HEIGHT  20
#define TB_MENU_WIDTH  100
#define TB_MAX_ITEMS     3

#define TB_ID_RESTORE    1
#define TB_ID_MINIMIZE   2
#define TB_ID_CLOSE      3

typedef struct {
    const char *text;
    uint8_t     id;
} tb_popup_item_t;

static bool   tb_popup_open_flag = false;
static hwnd_t tb_popup_hwnd = HWND_NULL;
static int8_t tb_popup_hover = -1;
static int16_t tb_popup_x, tb_popup_y;
static tb_popup_item_t tb_items[TB_MAX_ITEMS];
static int tb_item_count = 0;

bool taskbar_popup_is_open(void) {
    return tb_popup_open_flag;
}

void taskbar_popup_close(void) {
    tb_popup_open_flag = false;
    tb_popup_hwnd = HWND_NULL;
    tb_popup_hover = -1;
    wm_mark_dirty();
}

bool taskbar_mouse_rclick(int16_t x, int16_t y) {
    if (y < TASKBAR_Y) return false;

    int btn_x = TASK_AREA_X;
    int btn_w = taskbar_btn_width();

    for (uint8_t i = 1; i <= WM_MAX_WINDOWS; i++) {
        window_t *win = wm_get_window(i);
        if (!win || !(win->flags & WF_ALIVE) || !(win->flags & WF_BORDER))
            continue;

        if (x >= btn_x && x < btn_x + btn_w &&
            y >= BUTTON_Y && y < BUTTON_Y + BUTTON_HEIGHT) {
            /* Build popup items for this window */
            tb_popup_hwnd = (hwnd_t)i;
            tb_item_count = 0;

            if (win->state == WS_MINIMIZED) {
                tb_items[tb_item_count++] = (tb_popup_item_t){ "Restore", TB_ID_RESTORE };
            } else {
                tb_items[tb_item_count++] = (tb_popup_item_t){ "Minimize", TB_ID_MINIMIZE };
            }
            tb_items[tb_item_count++] = (tb_popup_item_t){ "Close", TB_ID_CLOSE };

            /* Position popup above the clicked button */
            int menu_h = 4 + tb_item_count * TB_ITEM_HEIGHT;
            tb_popup_x = btn_x;
            tb_popup_y = TASKBAR_Y - menu_h;
            if (tb_popup_x + TB_MENU_WIDTH > DISPLAY_WIDTH)
                tb_popup_x = DISPLAY_WIDTH - TB_MENU_WIDTH;

            tb_popup_hover = 0;
            tb_popup_open_flag = true;
            wm_mark_dirty();
            return true;
        }

        btn_x += btn_w + TASK_BUTTON_PAD;
    }

    return true; /* consumed (was in taskbar area) */
}

static void tb_popup_execute(uint8_t id) {
    hwnd_t hwnd = tb_popup_hwnd;
    taskbar_popup_close();

    switch (id) {
    case TB_ID_RESTORE:
        wm_restore_window(hwnd);
        wm_set_focus(hwnd);
        break;
    case TB_ID_MINIMIZE:
        wm_minimize_window(hwnd);
        break;
    case TB_ID_CLOSE:
        if (swap_is_suspended(hwnd)) {
            /* Suspended app can't process WM_CLOSE — force-close it */
            swap_force_close(hwnd);
        } else {
            window_event_t ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = WM_CLOSE;
            wm_post_event(hwnd, &ev);
        }
        break;
    }
    wm_mark_dirty();
}

void taskbar_popup_draw(void) {
    if (!tb_popup_open_flag) return;

    int menu_h = 4 + tb_item_count * TB_ITEM_HEIGHT;

    /* Shadow */
    gfx_fill_rect_dithered(tb_popup_x + 1, tb_popup_y + 1,
                           TB_MENU_WIDTH, menu_h, COLOR_BLACK);

    /* Background */
    gfx_fill_rect(tb_popup_x, tb_popup_y, TB_MENU_WIDTH, menu_h, THEME_BUTTON_FACE);

    /* Border */
    gfx_hline(tb_popup_x, tb_popup_y, TB_MENU_WIDTH, COLOR_WHITE);
    gfx_vline(tb_popup_x, tb_popup_y, menu_h, COLOR_WHITE);
    gfx_hline(tb_popup_x, tb_popup_y + menu_h - 1, TB_MENU_WIDTH, COLOR_BLACK);
    gfx_vline(tb_popup_x + TB_MENU_WIDTH - 1, tb_popup_y, menu_h, COLOR_BLACK);
    gfx_hline(tb_popup_x + 1, tb_popup_y + menu_h - 2, TB_MENU_WIDTH - 2, COLOR_DARK_GRAY);
    gfx_vline(tb_popup_x + TB_MENU_WIDTH - 2, tb_popup_y + 1, menu_h - 2, COLOR_DARK_GRAY);

    /* Items */
    int iy = tb_popup_y + 2;
    for (int i = 0; i < tb_item_count; i++) {
        bool hovered = (i == tb_popup_hover);
        uint8_t bg = hovered ? COLOR_BLUE : THEME_BUTTON_FACE;
        uint8_t fg = hovered ? COLOR_WHITE : COLOR_BLACK;

        gfx_fill_rect(tb_popup_x + 2, iy, TB_MENU_WIDTH - 4, TB_ITEM_HEIGHT, bg);
        gfx_text_ui(tb_popup_x + 6, iy + (TB_ITEM_HEIGHT - FONT_UI_HEIGHT) / 2,
                    tb_items[i].text, fg, bg);
        iy += TB_ITEM_HEIGHT;
    }
}

bool taskbar_popup_mouse(uint8_t type, int16_t x, int16_t y) {
    if (!tb_popup_open_flag) return false;

    int menu_h = 4 + tb_item_count * TB_ITEM_HEIGHT;

    if (x >= tb_popup_x && x < tb_popup_x + TB_MENU_WIDTH &&
        y >= tb_popup_y && y < tb_popup_y + menu_h) {
        if (type == WM_MOUSEMOVE || type == WM_LBUTTONDOWN) {
            int iy = tb_popup_y + 2;
            int new_hover = -1;
            for (int i = 0; i < tb_item_count; i++) {
                if (y >= iy && y < iy + TB_ITEM_HEIGHT) {
                    new_hover = i;
                    break;
                }
                iy += TB_ITEM_HEIGHT;
            }
            if (new_hover != tb_popup_hover) {
                tb_popup_hover = new_hover;
                wm_mark_dirty();
            }
        }
        if (type == WM_LBUTTONUP && tb_popup_hover >= 0) {
            tb_popup_execute(tb_items[tb_popup_hover].id);
        }
        return true;
    }

    /* Click outside — close */
    if (type == WM_LBUTTONDOWN || type == WM_RBUTTONDOWN) {
        taskbar_popup_close();
        return false;
    }
    return false;
}

bool taskbar_popup_handle_key(uint8_t hid_code, uint8_t modifiers) {
    if (!tb_popup_open_flag) return false;
    (void)modifiers;

    switch (hid_code) {
    case 0x52: /* UP */
        tb_popup_hover--;
        if (tb_popup_hover < 0) tb_popup_hover = tb_item_count - 1;
        wm_mark_dirty();
        return true;
    case 0x51: /* DOWN */
        tb_popup_hover++;
        if (tb_popup_hover >= tb_item_count) tb_popup_hover = 0;
        wm_mark_dirty();
        return true;
    case 0x28: /* ENTER */
        if (tb_popup_hover >= 0) tb_popup_execute(tb_items[tb_popup_hover].id);
        return true;
    case 0x29: /* ESC */
        taskbar_popup_close();
        return true;
    }
    return false;
}
