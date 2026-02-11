/*
 * FRANK OS
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dialog.h"
#include "window.h"
#include "window_event.h"
#include "window_theme.h"
#include "window_draw.h"
#include "display.h"
#include "gfx.h"
#include "font.h"
#include "taskbar.h"
#include <string.h>

/*==========================================================================
 * Icon data (defined in dialog_icons.c)
 *=========================================================================*/

extern const uint8_t icon_info_32x32[1024];
extern const uint8_t icon_warning_32x32[1024];
extern const uint8_t icon_error_32x32[1024];

/*==========================================================================
 * Layout constants (Win95 MessageBox style)
 *=========================================================================*/

#define DLG_ICON_SIZE       32
#define DLG_ICON_LEFT       12
#define DLG_ICON_TEXT_GAP   12
#define DLG_TEXT_TOP        12
#define DLG_TEXT_RIGHT      12
#define DLG_BTN_W           75
#define DLG_BTN_H           23
#define DLG_BTN_GAP          6
#define DLG_BTN_BOTTOM      10
#define DLG_BTN_TOP_PAD     12

/*==========================================================================
 * Internal state (single dialog at a time — modal)
 *=========================================================================*/

static hwnd_t      dlg_hwnd      = HWND_NULL;
static hwnd_t      dlg_parent    = HWND_NULL;
static uint8_t     dlg_icon;
static uint8_t     dlg_buttons;
static uint8_t     dlg_btn_count;
static uint8_t     dlg_btn_focus;
static uint16_t    dlg_btn_ids[4];
static const char *dlg_text;

/* Cached layout values (computed once in dialog_show) */
static int16_t     dlg_text_x;
static int16_t     dlg_client_w;
static int16_t     dlg_client_h;

/*==========================================================================
 * Icon drawing
 *=========================================================================*/

static void dialog_draw_icon(hwnd_t hwnd, int x, int y, uint8_t icon_type) {
    const uint8_t *data = NULL;
    switch (icon_type) {
        case DLG_ICON_INFO:    data = icon_info_32x32;    break;
        case DLG_ICON_WARNING: data = icon_warning_32x32; break;
        case DLG_ICON_ERROR:   data = icon_error_32x32;   break;
        default: return;
    }

    /* Get screen coordinates of client origin */
    window_t *win = wm_get_window(hwnd);
    if (!win) return;
    point_t co = theme_client_origin(&win->frame, win->flags);

    for (int row = 0; row < 32; row++) {
        for (int col = 0; col < 32; col++) {
            uint8_t c = data[row * 32 + col];
            if (c != 0xFF) {
                int sx = co.x + x + col;
                int sy = co.y + y + row;
                if ((unsigned)sx < (unsigned)DISPLAY_WIDTH &&
                    (unsigned)sy < (unsigned)FB_HEIGHT)
                    display_set_pixel(sx, sy, c);
            }
        }
    }
}

/*==========================================================================
 * Button drawing
 *=========================================================================*/

static const char *btn_label(uint16_t result_id) {
    switch (result_id) {
        case DLG_RESULT_OK:     return "OK";
        case DLG_RESULT_CANCEL: return "Cancel";
        case DLG_RESULT_YES:    return "Yes";
        case DLG_RESULT_NO:     return "No";
        default: return "";
    }
}

/* Screen-coordinate origin of the dialog client area — cached per paint */
static int dlg_screen_ox, dlg_screen_oy;

static void draw_dialog_button(int16_t x, int16_t y, int16_t w, int16_t h,
                                const char *label, bool focused) {
    /* Raised bevel button */
    wd_fill_rect(x, y, w, h, THEME_BUTTON_FACE);
    /* Outer bevel */
    wd_hline(x, y, w, COLOR_WHITE);
    wd_vline(x, y, h, COLOR_WHITE);
    wd_hline(x, y + h - 1, w, COLOR_BLACK);
    wd_vline(x + w - 1, y, h, COLOR_BLACK);
    /* Inner shadow */
    wd_hline(x + 1, y + h - 2, w - 2, COLOR_DARK_GRAY);
    wd_vline(x + w - 2, y + 1, h - 2, COLOR_DARK_GRAY);

    /* Center label text using UI font (screen coordinates) */
    int text_w = (int)strlen(label) * FONT_UI_WIDTH;
    int tx = dlg_screen_ox + x + (w - text_w) / 2;
    int ty = dlg_screen_oy + y + (h - FONT_UI_HEIGHT) / 2;
    gfx_text_ui(tx, ty, label, COLOR_BLACK, THEME_BUTTON_FACE);

    /* Focus indicator: dotted rectangle 3px inside button */
    if (focused) {
        int fx = x + 4;
        int fy = y + 4;
        int fw = w - 8;
        int fh = h - 8;
        for (int i = fx; i < fx + fw; i += 2) {
            wd_pixel(i, fy, COLOR_BLACK);
            wd_pixel(i, fy + fh - 1, COLOR_BLACK);
        }
        for (int j = fy; j < fy + fh; j += 2) {
            wd_pixel(fx, j, COLOR_BLACK);
            wd_pixel(fx + fw - 1, j, COLOR_BLACK);
        }
    }
}

/*==========================================================================
 * Paint handler
 *=========================================================================*/

static void dialog_paint(hwnd_t hwnd) {
    /* Cache screen-coordinate origin for UI font drawing */
    window_t *win = wm_get_window(hwnd);
    if (win) {
        point_t co = theme_client_origin(&win->frame, win->flags);
        dlg_screen_ox = co.x;
        dlg_screen_oy = co.y;
    }

    /* Icon */
    if (dlg_icon != DLG_ICON_NONE) {
        dialog_draw_icon(hwnd, DLG_ICON_LEFT, DLG_TEXT_TOP, dlg_icon);
    }

    /* Text — draw line by line using UI font (screen coordinates) */
    {
        int tx = dlg_screen_ox + dlg_text_x;
        int ty = dlg_screen_oy + DLG_TEXT_TOP;
        const char *p = dlg_text;
        char line[128];

        while (p && *p) {
            const char *nl = strchr(p, '\n');
            int len;
            if (nl) {
                len = (int)(nl - p);
                if (len >= (int)sizeof(line)) len = sizeof(line) - 1;
                memcpy(line, p, len);
                line[len] = '\0';
                p = nl + 1;
            } else {
                len = (int)strlen(p);
                if (len >= (int)sizeof(line)) len = sizeof(line) - 1;
                memcpy(line, p, len);
                line[len] = '\0';
                p = NULL;
            }
            gfx_text_ui(tx, ty, line, COLOR_BLACK, THEME_BUTTON_FACE);
            ty += FONT_UI_HEIGHT;
        }
    }

    /* Separator line above buttons */
    {
        int sep_y = dlg_client_h - DLG_BTN_H - DLG_BTN_BOTTOM - DLG_BTN_TOP_PAD / 2;
        wd_hline(0, sep_y, dlg_client_w, COLOR_DARK_GRAY);
        wd_hline(0, sep_y + 1, dlg_client_w, COLOR_WHITE);
    }

    /* Buttons — centered at bottom */
    {
        int total_btn_w = dlg_btn_count * DLG_BTN_W +
                          (dlg_btn_count - 1) * DLG_BTN_GAP;
        int bx = (dlg_client_w - total_btn_w) / 2;
        int by = dlg_client_h - DLG_BTN_H - DLG_BTN_BOTTOM;

        for (int i = 0; i < dlg_btn_count; i++) {
            draw_dialog_button(bx, by, DLG_BTN_W, DLG_BTN_H,
                               btn_label(dlg_btn_ids[i]),
                               i == dlg_btn_focus);
            bx += DLG_BTN_W + DLG_BTN_GAP;
        }
    }
}

/*==========================================================================
 * Close/activate logic
 *=========================================================================*/

static void dialog_close(uint16_t result) {
    wm_clear_modal();
    wm_destroy_window(dlg_hwnd);
    dlg_hwnd = HWND_NULL;

    /* Restore focus to parent window (must happen after modal is cleared
     * and dialog is destroyed, otherwise wm_set_focus refuses the change) */
    if (dlg_parent != HWND_NULL) {
        wm_set_focus(dlg_parent);

        window_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = WM_COMMAND;
        ev.command.id = result;
        wm_post_event(dlg_parent, &ev);
    }
}

/*==========================================================================
 * Event handler
 *=========================================================================*/

static bool dialog_event(hwnd_t hwnd, const window_event_t *event) {
    (void)hwnd;

    switch (event->type) {
    case WM_CLOSE:
        /* Treat close as Cancel if available, otherwise OK */
        if (dlg_buttons & DLG_BTN_CANCEL)
            dialog_close(DLG_RESULT_CANCEL);
        else
            dialog_close(dlg_btn_ids[0]);
        return true;

    case WM_KEYDOWN:
        /* Scancodes here are HID usage codes (set by main.c input_task) */
        switch (event->key.scancode) {
        case 0x28: /* HID Enter */
            dialog_close(dlg_btn_ids[dlg_btn_focus]);
            return true;
        case 0x29: /* HID Escape */
            if (dlg_buttons & DLG_BTN_CANCEL)
                dialog_close(DLG_RESULT_CANCEL);
            else
                dialog_close(dlg_btn_ids[0]);
            return true;
        case 0x2B: /* HID Tab */
            dlg_btn_focus = (dlg_btn_focus + 1) % dlg_btn_count;
            wm_invalidate(dlg_hwnd);
            return true;
        case 0x50: /* HID Left Arrow */
            if (dlg_btn_focus > 0) dlg_btn_focus--;
            wm_invalidate(dlg_hwnd);
            return true;
        case 0x4F: /* HID Right Arrow */
            if (dlg_btn_focus < dlg_btn_count - 1) dlg_btn_focus++;
            wm_invalidate(dlg_hwnd);
            return true;
        }
        return false;

    case WM_LBUTTONDOWN: {
        /* Hit-test buttons */
        int total_btn_w = dlg_btn_count * DLG_BTN_W +
                          (dlg_btn_count - 1) * DLG_BTN_GAP;
        int bx = (dlg_client_w - total_btn_w) / 2;
        int by = dlg_client_h - DLG_BTN_H - DLG_BTN_BOTTOM;
        int mx = event->mouse.x;
        int my = event->mouse.y;

        if (my >= by && my < by + DLG_BTN_H) {
            for (int i = 0; i < dlg_btn_count; i++) {
                if (mx >= bx && mx < bx + DLG_BTN_W) {
                    dialog_close(dlg_btn_ids[i]);
                    return true;
                }
                bx += DLG_BTN_W + DLG_BTN_GAP;
            }
        }
        return true;
    }

    default:
        return false;
    }
}

/*==========================================================================
 * Public API
 *=========================================================================*/

hwnd_t dialog_show(hwnd_t parent, const char *title, const char *text,
                   uint8_t icon, uint8_t buttons) {
    /* Only one dialog at a time */
    if (dlg_hwnd != HWND_NULL) return HWND_NULL;

    dlg_parent  = parent;
    dlg_icon    = icon;
    dlg_buttons = buttons;
    dlg_text    = text;

    /* Build button list from flags */
    dlg_btn_count = 0;
    if (buttons & DLG_BTN_OK)     dlg_btn_ids[dlg_btn_count++] = DLG_RESULT_OK;
    if (buttons & DLG_BTN_YES)    dlg_btn_ids[dlg_btn_count++] = DLG_RESULT_YES;
    if (buttons & DLG_BTN_NO)     dlg_btn_ids[dlg_btn_count++] = DLG_RESULT_NO;
    if (buttons & DLG_BTN_CANCEL) dlg_btn_ids[dlg_btn_count++] = DLG_RESULT_CANCEL;
    if (dlg_btn_count == 0) {
        dlg_btn_ids[dlg_btn_count++] = DLG_RESULT_OK;
        dlg_buttons = DLG_BTN_OK;
    }
    dlg_btn_focus = 0;

    /* Measure text */
    int max_line_w = 0;
    int line_count = 0;
    {
        const char *p = text;
        while (p && *p) {
            const char *nl = strchr(p, '\n');
            int len;
            if (nl) {
                len = (int)(nl - p);
                p = nl + 1;
            } else {
                len = (int)strlen(p);
                p = NULL;
            }
            int w = len * FONT_UI_WIDTH;
            if (w > max_line_w) max_line_w = w;
            line_count++;
        }
        /* Handle empty text */
        if (line_count == 0) line_count = 1;
    }

    int text_height = line_count * FONT_UI_HEIGHT;

    /* Text X position */
    if (icon != DLG_ICON_NONE) {
        dlg_text_x = DLG_ICON_LEFT + DLG_ICON_SIZE + DLG_ICON_TEXT_GAP;
    } else {
        dlg_text_x = DLG_TEXT_RIGHT;
    }

    /* Content dimensions */
    int content_w = dlg_text_x + max_line_w + DLG_TEXT_RIGHT;
    int icon_text_h = (icon != DLG_ICON_NONE && DLG_ICON_SIZE > text_height) ?
                      DLG_ICON_SIZE : text_height;
    int content_h = DLG_TEXT_TOP + icon_text_h + DLG_BTN_TOP_PAD +
                    DLG_BTN_H + DLG_BTN_BOTTOM;

    /* Button row width */
    int btn_row_w = dlg_btn_count * DLG_BTN_W +
                    (dlg_btn_count - 1) * DLG_BTN_GAP;

    /* Client area dimensions */
    int client_w = content_w;
    if (btn_row_w + 2 * DLG_TEXT_RIGHT > client_w)
        client_w = btn_row_w + 2 * DLG_TEXT_RIGHT;

    /* Minimum width */
    if (client_w < 200) client_w = 200;

    dlg_client_w = (int16_t)client_w;
    dlg_client_h = (int16_t)content_h;

    /* Outer dimensions including decorations */
    int outer_w = client_w + 2 * THEME_BORDER_WIDTH;
    int outer_h = content_h + THEME_TITLE_HEIGHT + 2 * THEME_BORDER_WIDTH;

    /* Center on screen (above taskbar) */
    int work_h = taskbar_work_area_height();
    int x = (DISPLAY_WIDTH - outer_w) / 2;
    int y = (work_h - outer_h) / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    /* Create the window */
    dlg_hwnd = wm_create_window((int16_t)x, (int16_t)y,
                                 (int16_t)outer_w, (int16_t)outer_h,
                                 title, WSTYLE_DIALOG,
                                 dialog_event, dialog_paint);
    if (dlg_hwnd == HWND_NULL) return HWND_NULL;

    /* Set button face background */
    window_t *win = wm_get_window(dlg_hwnd);
    if (win) {
        win->bg_color = THEME_BUTTON_FACE;
    }

    wm_set_focus(dlg_hwnd);
    wm_set_modal(dlg_hwnd);

    return dlg_hwnd;
}
