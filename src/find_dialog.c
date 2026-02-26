/*
 * FRANK OS — Find/Replace Dialog
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * Modeless dialog following the dialog.c input pattern.
 * Two text fields, Match Case checkbox, and action buttons.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "find_dialog.h"
#include "window.h"
#include "window_event.h"
#include "window_draw.h"
#include "window_theme.h"
#include "display.h"
#include "gfx.h"
#include "font.h"
#include "taskbar.h"
#include <string.h>
#include "FreeRTOS.h"
#include "timers.h"

/*==========================================================================
 * Constants
 *=========================================================================*/

#define FND_FIND_BUF_MAX    64
#define FND_REPL_BUF_MAX    64

/* Layout */
#define FND_PAD              8
#define FND_LABEL_W         90
#define FND_FIELD_H         20
#define FND_FIELD_GAP        4
#define FND_BTN_W           90
#define FND_BTN_H           23
#define FND_BTN_GAP          4
#define FND_CHECK_SIZE      13

/* Find-only client area */
#define FND_FIND_CLIENT_W   360
#define FND_FIND_CLIENT_H   100

/* Replace client area (taller) */
#define FND_REPL_CLIENT_W   360
#define FND_REPL_CLIENT_H   130

/*==========================================================================
 * Static state (single dialog)
 *=========================================================================*/

static hwnd_t   fnd_hwnd   = HWND_NULL;
static hwnd_t   fnd_parent = HWND_NULL;
static bool     fnd_replace_mode;

/* Text fields */
static char     fnd_find_buf[FND_FIND_BUF_MAX + 1];
static uint8_t  fnd_find_len;
static uint8_t  fnd_find_cursor;

static char     fnd_repl_buf[FND_REPL_BUF_MAX + 1];
static uint8_t  fnd_repl_len;
static uint8_t  fnd_repl_cursor;

/* Focus: 0 = find field, 1 = replace field, 2+ = buttons */
static int8_t   fnd_focus;
static bool     fnd_match_case;

/* Cursor blink */
static bool          fnd_cursor_visible;
static TimerHandle_t fnd_blink_timer = NULL;

/* Button press state */
static int8_t   fnd_btn_pressed;  /* -1 = none */

/* Cached layout */
static int16_t  fnd_client_w;
static int16_t  fnd_client_h;
static int16_t  fnd_field_x;
static int16_t  fnd_field_w;

/*==========================================================================
 * Blink timer
 *=========================================================================*/

static void fnd_blink_callback(TimerHandle_t xTimer) {
    (void)xTimer;
    fnd_cursor_visible = !fnd_cursor_visible;
    wm_invalidate(fnd_hwnd);
}

static void fnd_blink_reset(void) {
    fnd_cursor_visible = true;
    if (fnd_blink_timer) xTimerReset(fnd_blink_timer, 0);
    wm_invalidate(fnd_hwnd);
}

/*==========================================================================
 * Sunken field
 *=========================================================================*/

static void fnd_draw_sunken(int16_t x, int16_t y, int16_t w, int16_t h) {
    wd_hline(x, y, w, COLOR_DARK_GRAY);
    wd_vline(x, y, h, COLOR_DARK_GRAY);
    wd_hline(x + 1, y + 1, w - 2, COLOR_BLACK);
    wd_vline(x + 1, y + 1, h - 2, COLOR_BLACK);
    wd_hline(x, y + h - 1, w, COLOR_WHITE);
    wd_vline(x + w - 1, y, h, COLOR_WHITE);
    wd_hline(x + 1, y + h - 2, w - 2, THEME_BUTTON_FACE);
    wd_vline(x + w - 2, y + 1, h - 2, THEME_BUTTON_FACE);
    wd_fill_rect(x + 2, y + 2, w - 4, h - 4, COLOR_WHITE);
}

/*==========================================================================
 * Checkbox drawing
 *=========================================================================*/

static void fnd_draw_checkbox(int16_t x, int16_t y, bool checked) {
    /* Sunken box */
    wd_hline(x, y, FND_CHECK_SIZE, COLOR_DARK_GRAY);
    wd_vline(x, y, FND_CHECK_SIZE, COLOR_DARK_GRAY);
    wd_hline(x, y + FND_CHECK_SIZE - 1, FND_CHECK_SIZE, COLOR_WHITE);
    wd_vline(x + FND_CHECK_SIZE - 1, y, FND_CHECK_SIZE, COLOR_WHITE);
    wd_fill_rect(x + 1, y + 1, FND_CHECK_SIZE - 2, FND_CHECK_SIZE - 2,
                  COLOR_WHITE);

    if (checked) {
        /* Draw checkmark */
        int cx = x + 3, cy = y + 5;
        wd_pixel(cx, cy, COLOR_BLACK);
        wd_pixel(cx + 1, cy + 1, COLOR_BLACK);
        wd_pixel(cx + 2, cy + 2, COLOR_BLACK);
        wd_pixel(cx + 3, cy + 1, COLOR_BLACK);
        wd_pixel(cx + 4, cy, COLOR_BLACK);
        wd_pixel(cx + 5, cy - 1, COLOR_BLACK);
        wd_pixel(cx + 6, cy - 2, COLOR_BLACK);
        /* Thicken */
        wd_pixel(cx, cy - 1, COLOR_BLACK);
        wd_pixel(cx + 1, cy, COLOR_BLACK);
        wd_pixel(cx + 2, cy + 1, COLOR_BLACK);
        wd_pixel(cx + 3, cy, COLOR_BLACK);
        wd_pixel(cx + 4, cy - 1, COLOR_BLACK);
        wd_pixel(cx + 5, cy - 2, COLOR_BLACK);
        wd_pixel(cx + 6, cy - 3, COLOR_BLACK);
    }
}

/*==========================================================================
 * Layout helpers
 *=========================================================================*/

static int16_t fnd_find_field_y(void) { return FND_PAD; }
static int16_t fnd_repl_field_y(void) { return FND_PAD + FND_FIELD_H + FND_FIELD_GAP; }

static int16_t fnd_checkbox_y(void) {
    if (fnd_replace_mode)
        return fnd_repl_field_y() + FND_FIELD_H + FND_FIELD_GAP + 2;
    else
        return fnd_find_field_y() + FND_FIELD_H + FND_FIELD_GAP + 2;
}

static int16_t fnd_btn_x(void) {
    return fnd_client_w - FND_PAD - FND_BTN_W;
}

/* Button count: Find=2 (Find Next, Cancel), Replace=4 (Find Next, Replace, Replace All, Cancel) */
static int fnd_btn_count(void) {
    return fnd_replace_mode ? 4 : 2;
}

static const char *fnd_btn_label(int idx) {
    if (fnd_replace_mode) {
        switch (idx) {
            case 0: return "Find Next";
            case 1: return "Replace";
            case 2: return "Replace All";
            case 3: return "Cancel";
        }
    } else {
        switch (idx) {
            case 0: return "Find Next";
            case 1: return "Cancel";
        }
    }
    return "";
}

static int16_t fnd_btn_y(int idx) {
    return FND_PAD + idx * (FND_BTN_H + FND_BTN_GAP);
}

/* Map focus index to meaning: 0=find, 1=replace(if mode), then buttons */
static int fnd_first_btn_focus(void) {
    return fnd_replace_mode ? 2 : 1;
}

static int fnd_max_focus(void) {
    return fnd_first_btn_focus() + fnd_btn_count();
}

static bool fnd_is_field_focus(void) {
    return fnd_focus < fnd_first_btn_focus();
}

static int fnd_btn_index(void) {
    if (!fnd_is_field_focus())
        return fnd_focus - fnd_first_btn_focus();
    return -1;
}

/*==========================================================================
 * Paint
 *=========================================================================*/

static void fnd_paint(hwnd_t hwnd) {
    (void)hwnd;

    window_t *win = wm_get_window(fnd_hwnd);
    int sox = 0, soy = 0;
    if (win) {
        point_t co = theme_client_origin(&win->frame, win->flags);
        sox = co.x;
        soy = co.y;
    }

    /* "Find what:" field */
    wd_text_ui(FND_PAD, fnd_find_field_y() + (FND_FIELD_H - FONT_UI_HEIGHT) / 2,
               "Find what:", COLOR_BLACK, THEME_BUTTON_FACE);
    fnd_draw_sunken(fnd_field_x, fnd_find_field_y(), fnd_field_w, FND_FIELD_H);
    {
        int tx = sox + fnd_field_x + 4;
        int ty = soy + fnd_find_field_y() + (FND_FIELD_H - FONT_UI_HEIGHT) / 2;
        gfx_text_ui(tx, ty, fnd_find_buf, COLOR_BLACK, COLOR_WHITE);
        if (fnd_focus == 0 && fnd_cursor_visible) {
            int cx = tx + fnd_find_cursor * FONT_UI_WIDTH;
            for (int r = 0; r < FONT_UI_HEIGHT; r++)
                display_set_pixel(cx, ty + r, COLOR_BLACK);
        }
    }

    /* "Replace with:" field (replace mode only) */
    if (fnd_replace_mode) {
        wd_text_ui(FND_PAD,
                   fnd_repl_field_y() + (FND_FIELD_H - FONT_UI_HEIGHT) / 2,
                   "Replace with:", COLOR_BLACK, THEME_BUTTON_FACE);
        fnd_draw_sunken(fnd_field_x, fnd_repl_field_y(),
                        fnd_field_w, FND_FIELD_H);
        {
            int tx = sox + fnd_field_x + 4;
            int ty = soy + fnd_repl_field_y() + (FND_FIELD_H - FONT_UI_HEIGHT) / 2;
            gfx_text_ui(tx, ty, fnd_repl_buf, COLOR_BLACK, COLOR_WHITE);
            if (fnd_focus == 1 && fnd_cursor_visible) {
                int cx = tx + fnd_repl_cursor * FONT_UI_WIDTH;
                for (int r = 0; r < FONT_UI_HEIGHT; r++)
                    display_set_pixel(cx, ty + r, COLOR_BLACK);
            }
        }
    }

    /* "Match case" checkbox */
    int16_t cby = fnd_checkbox_y();
    fnd_draw_checkbox(FND_PAD, cby, fnd_match_case);
    wd_text_ui(FND_PAD + FND_CHECK_SIZE + 4,
               cby + (FND_CHECK_SIZE - FONT_UI_HEIGHT) / 2,
               "Match case", COLOR_BLACK, THEME_BUTTON_FACE);

    /* Buttons (right column) */
    int bc = fnd_btn_count();
    int bx = fnd_btn_x();
    int bi = fnd_btn_index();
    for (int i = 0; i < bc; i++) {
        wd_button(bx, fnd_btn_y(i), FND_BTN_W, FND_BTN_H,
                  fnd_btn_label(i),
                  !fnd_is_field_focus() && bi == i,
                  fnd_btn_pressed == i);
    }
}

/*==========================================================================
 * Close
 *=========================================================================*/

static void fnd_do_close(void) {
    if (fnd_blink_timer) {
        xTimerStop(fnd_blink_timer, 0);
        xTimerDelete(fnd_blink_timer, 0);
        fnd_blink_timer = NULL;
    }
    wm_destroy_window(fnd_hwnd);
    fnd_hwnd = HWND_NULL;
    if (fnd_parent != HWND_NULL)
        wm_set_focus(fnd_parent);
}

/*==========================================================================
 * Post result to parent
 *=========================================================================*/

static void fnd_post_result(uint16_t result) {
    if (fnd_parent != HWND_NULL) {
        window_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = WM_COMMAND;
        ev.command.id = result;
        wm_post_event(fnd_parent, &ev);
    }
}

/*==========================================================================
 * Button action dispatch
 *=========================================================================*/

static void fnd_activate_button(int idx) {
    if (fnd_replace_mode) {
        switch (idx) {
            case 0: fnd_post_result(DLG_RESULT_FIND_NEXT); fnd_do_close(); return;
            case 1: fnd_post_result(DLG_RESULT_REPLACE); fnd_do_close(); return;
            case 2: fnd_post_result(DLG_RESULT_REPLACE_ALL); fnd_do_close(); return;
            case 3: fnd_do_close(); return;
        }
    } else {
        switch (idx) {
            case 0: fnd_post_result(DLG_RESULT_FIND_NEXT); fnd_do_close(); return;
            case 1: fnd_do_close(); return;
        }
    }
}

/*==========================================================================
 * Text field editing helpers
 *=========================================================================*/

static void fnd_field_char(char *buf, uint8_t *len, uint8_t *cursor,
                            uint8_t max_len, char ch) {
    if (*len < max_len) {
        for (int i = *len; i > *cursor; i--)
            buf[i] = buf[i - 1];
        buf[*cursor] = ch;
        (*len)++;
        (*cursor)++;
        buf[*len] = '\0';
        fnd_blink_reset();
    }
}

static void fnd_field_backspace(char *buf, uint8_t *len, uint8_t *cursor) {
    if (*cursor > 0) {
        for (int i = *cursor - 1; i < *len - 1; i++)
            buf[i] = buf[i + 1];
        (*len)--;
        (*cursor)--;
        buf[*len] = '\0';
        fnd_blink_reset();
    }
}

static void fnd_field_delete(char *buf, uint8_t *len, uint8_t *cursor) {
    if (*cursor < *len) {
        for (int i = *cursor; i < *len - 1; i++)
            buf[i] = buf[i + 1];
        (*len)--;
        buf[*len] = '\0';
        fnd_blink_reset();
    }
}

/*==========================================================================
 * Event handler
 *=========================================================================*/

static bool fnd_event(hwnd_t hwnd, const window_event_t *event) {
    (void)hwnd;

    switch (event->type) {
    case WM_CLOSE:
        fnd_do_close();
        return true;

    case WM_CHAR:
        if (fnd_is_field_focus()) {
            char ch = event->charev.ch;
            if (ch < 0x20 || ch >= 0x7F) return true;
            if (event->charev.modifiers & KMOD_CTRL) return true;
            if (fnd_focus == 0) {
                fnd_field_char(fnd_find_buf, &fnd_find_len,
                               &fnd_find_cursor, FND_FIND_BUF_MAX, ch);
            } else if (fnd_focus == 1 && fnd_replace_mode) {
                fnd_field_char(fnd_repl_buf, &fnd_repl_len,
                               &fnd_repl_cursor, FND_REPL_BUF_MAX, ch);
            }
            return true;
        }
        return false;

    case WM_KEYDOWN: {
        uint8_t sc = event->key.scancode;

        /* Escape always closes */
        if (sc == 0x29) {
            fnd_do_close();
            return true;
        }

        /* Enter — find next or activate focused button */
        if (sc == 0x28) {
            if (fnd_is_field_focus())
                fnd_activate_button(0); /* Find Next */
            else
                fnd_activate_button(fnd_btn_index());
            return true;
        }

        /* Tab — cycle focus */
        if (sc == 0x2B) {
            fnd_focus = (fnd_focus + 1) % fnd_max_focus();
            wm_invalidate(fnd_hwnd);
            return true;
        }

        /* Field editing keys */
        if (fnd_is_field_focus()) {
            char *buf;
            uint8_t *len, *cursor;
            uint8_t max_len;
            if (fnd_focus == 0) {
                buf = fnd_find_buf;
                len = &fnd_find_len;
                cursor = &fnd_find_cursor;
                max_len = FND_FIND_BUF_MAX;
            } else {
                buf = fnd_repl_buf;
                len = &fnd_repl_len;
                cursor = &fnd_repl_cursor;
                max_len = FND_REPL_BUF_MAX;
            }
            (void)max_len;

            switch (sc) {
            case 0x2A: /* Backspace */
                fnd_field_backspace(buf, len, cursor);
                return true;
            case 0x4C: /* Delete */
                fnd_field_delete(buf, len, cursor);
                return true;
            case 0x50: /* Left */
                if (*cursor > 0) { (*cursor)--; fnd_blink_reset(); }
                return true;
            case 0x4F: /* Right */
                if (*cursor < *len) { (*cursor)++; fnd_blink_reset(); }
                return true;
            case 0x4A: /* Home */
                *cursor = 0;
                fnd_blink_reset();
                return true;
            case 0x4D: /* End */
                *cursor = *len;
                fnd_blink_reset();
                return true;
            }
        }
        return false;
    }

    case WM_LBUTTONDOWN: {
        int mx = event->mouse.x;
        int my = event->mouse.y;

        fnd_btn_pressed = -1;

        /* Hit-test: Find field */
        if (mx >= fnd_field_x + 2 && mx < fnd_field_x + fnd_field_w - 2 &&
            my >= fnd_find_field_y() &&
            my < fnd_find_field_y() + FND_FIELD_H) {
            fnd_focus = 0;
            int click_x = mx - fnd_field_x - 4;
            int new_pos = click_x / FONT_UI_WIDTH;
            if (new_pos < 0) new_pos = 0;
            if (new_pos > fnd_find_len) new_pos = fnd_find_len;
            fnd_find_cursor = new_pos;
            fnd_blink_reset();
            return true;
        }

        /* Hit-test: Replace field */
        if (fnd_replace_mode &&
            mx >= fnd_field_x + 2 && mx < fnd_field_x + fnd_field_w - 2 &&
            my >= fnd_repl_field_y() &&
            my < fnd_repl_field_y() + FND_FIELD_H) {
            fnd_focus = 1;
            int click_x = mx - fnd_field_x - 4;
            int new_pos = click_x / FONT_UI_WIDTH;
            if (new_pos < 0) new_pos = 0;
            if (new_pos > fnd_repl_len) new_pos = fnd_repl_len;
            fnd_repl_cursor = new_pos;
            fnd_blink_reset();
            return true;
        }

        /* Hit-test: Match case checkbox */
        {
            int16_t cby = fnd_checkbox_y();
            if (mx >= FND_PAD && mx < FND_PAD + FND_CHECK_SIZE + 70 &&
                my >= cby && my < cby + FND_CHECK_SIZE) {
                fnd_match_case = !fnd_match_case;
                wm_invalidate(fnd_hwnd);
                return true;
            }
        }

        /* Hit-test: Buttons */
        {
            int bc = fnd_btn_count();
            int bx = fnd_btn_x();
            for (int i = 0; i < bc; i++) {
                int by = fnd_btn_y(i);
                if (mx >= bx && mx < bx + FND_BTN_W &&
                    my >= by && my < by + FND_BTN_H) {
                    fnd_btn_pressed = i;
                    fnd_focus = fnd_first_btn_focus() + i;
                    wm_invalidate(fnd_hwnd);
                    return true;
                }
            }
        }
        return true;
    }

    case WM_LBUTTONUP: {
        if (fnd_btn_pressed < 0) return true;
        int pressed = fnd_btn_pressed;
        fnd_btn_pressed = -1;

        int bx = fnd_btn_x();
        int by = fnd_btn_y(pressed);
        int mx = event->mouse.x;
        int my = event->mouse.y;

        if (mx >= bx && mx < bx + FND_BTN_W &&
            my >= by && my < by + FND_BTN_H) {
            fnd_activate_button(pressed);
        } else {
            wm_invalidate(fnd_hwnd);
        }
        return true;
    }

    default:
        return false;
    }
}

/*==========================================================================
 * Internal open helper
 *=========================================================================*/

static hwnd_t fnd_open(hwnd_t parent, bool replace_mode) {
    /* If already open, just switch mode and refocus */
    if (fnd_hwnd != HWND_NULL) {
        if (fnd_replace_mode != replace_mode) {
            /* Need to recreate with different size */
            fnd_do_close();
        } else {
            wm_set_focus(fnd_hwnd);
            return fnd_hwnd;
        }
    }

    fnd_parent = parent;
    fnd_replace_mode = replace_mode;
    fnd_focus = 0;
    fnd_btn_pressed = -1;

    /* Keep existing search text if any */

    if (replace_mode) {
        fnd_client_w = FND_REPL_CLIENT_W;
        fnd_client_h = FND_REPL_CLIENT_H;
    } else {
        fnd_client_w = FND_FIND_CLIENT_W;
        fnd_client_h = FND_FIND_CLIENT_H;
    }

    fnd_field_x = FND_LABEL_W;
    fnd_field_w = fnd_client_w - FND_LABEL_W - FND_PAD - FND_BTN_W - FND_PAD;

    int outer_w = fnd_client_w + 2 * THEME_BORDER_WIDTH;
    int outer_h = fnd_client_h + THEME_TITLE_HEIGHT + 2 * THEME_BORDER_WIDTH;

    /* Position near top-right of screen */
    int work_h = taskbar_work_area_height();
    int x = DISPLAY_WIDTH - outer_w - 20;
    int y = 40;
    if (x < 0) x = 0;
    if (y + outer_h > work_h) y = work_h - outer_h;
    if (y < 0) y = 0;

    const char *title = replace_mode ? "Replace" : "Find";
    fnd_hwnd = wm_create_window((int16_t)x, (int16_t)y,
                                  (int16_t)outer_w, (int16_t)outer_h,
                                  title, WSTYLE_DIALOG,
                                  fnd_event, fnd_paint);
    if (fnd_hwnd == HWND_NULL) return HWND_NULL;

    window_t *win = wm_get_window(fnd_hwnd);
    if (win) win->bg_color = THEME_BUTTON_FACE;

    wm_set_focus(fnd_hwnd);
    /* Note: NOT modal — user can still interact with parent */

    /* Cursor blink timer */
    fnd_cursor_visible = true;
    fnd_blink_timer = xTimerCreate("fndblink", pdMS_TO_TICKS(500),
                                     pdTRUE, NULL, fnd_blink_callback);
    if (fnd_blink_timer) xTimerStart(fnd_blink_timer, 0);

    return fnd_hwnd;
}

/*==========================================================================
 * Public API
 *=========================================================================*/

hwnd_t find_dialog_show(hwnd_t parent) {
    return fnd_open(parent, false);
}

hwnd_t replace_dialog_show(hwnd_t parent) {
    return fnd_open(parent, true);
}

const char *find_dialog_get_text(void) {
    return fnd_find_buf;
}

const char *find_dialog_get_replace_text(void) {
    return fnd_repl_buf;
}

bool find_dialog_case_sensitive(void) {
    return fnd_match_case;
}

void find_dialog_close(void) {
    if (fnd_hwnd != HWND_NULL)
        fnd_do_close();
}
