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
#include "gfx.h"
#include "font.h"
#include "display.h"
#include <string.h>

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
    wm_mark_dirty();
}

void taskbar_draw(void) {
    if (!taskbar_ready) return;

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
    int max_x = DISPLAY_WIDTH - 4;

    for (uint8_t i = 1; i <= WM_MAX_WINDOWS; i++) {
        window_t *win = wm_get_window(i);
        if (!win || !(win->flags & WF_ALIVE)) continue;
        if (!(win->flags & WF_BORDER)) continue; /* skip borderless popups */

        /* Calculate button width: fit remaining space or use max width */
        int btn_w = TASK_BUTTON_W;
        if (btn_x + btn_w > max_x) btn_w = max_x - btn_x;
        if (btn_w < 30) break; /* no room for more buttons */

        bool is_focused = (i == focus) && (win->flags & WF_VISIBLE);

        if (is_focused) {
            draw_sunken_button(btn_x, BUTTON_Y, btn_w, BUTTON_HEIGHT);
        } else {
            draw_raised_button(btn_x, BUTTON_Y, btn_w, BUTTON_HEIGHT);
        }

        /* Truncated title text */
        int text_x = btn_x + 4 + (is_focused ? 1 : 0);
        int text_y = BUTTON_Y + (BUTTON_HEIGHT - FONT_UI_HEIGHT) / 2 +
                     (is_focused ? 1 : 0);
        int text_max_w = btn_w - 8;
        if (text_max_w > 0) {
            gfx_text_ui_clipped(text_x, text_y, win->title,
                                COLOR_BLACK, THEME_BUTTON_FACE,
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
        wm_mark_dirty();
        return true;
    }

    /* Per-window task buttons */
    int btn_x = TASK_AREA_X;
    int max_x = DISPLAY_WIDTH - 4;

    for (uint8_t i = 1; i <= WM_MAX_WINDOWS; i++) {
        window_t *win = wm_get_window(i);
        if (!win || !(win->flags & WF_ALIVE)) continue;
        if (!(win->flags & WF_BORDER)) continue;

        int btn_w = TASK_BUTTON_W;
        if (btn_x + btn_w > max_x) btn_w = max_x - btn_x;
        if (btn_w < 30) break;

        if (x >= btn_x && x < btn_x + btn_w &&
            y >= BUTTON_Y && y < BUTTON_Y + BUTTON_HEIGHT) {
            hwnd_t hwnd = (hwnd_t)i;
            if (win->state == WS_MINIMIZED) {
                wm_restore_window(hwnd);
            }
            wm_set_focus(hwnd);
            wm_mark_dirty();
            return true;
        }

        btn_x += btn_w + TASK_BUTTON_PAD;
    }

    return true; /* consumed the click (was in taskbar area) */
}
