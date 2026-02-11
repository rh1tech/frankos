/*
 * FRANK OS
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WINDOW_THEME_H
#define WINDOW_THEME_H

#include "window.h"

/*==========================================================================
 * Decoration metrics (pixels) — Win95 style
 *=========================================================================*/

#define THEME_TITLE_HEIGHT   20    /* title bar height */
#define THEME_BORDER_WIDTH    4    /* border thickness (Win95 double bevel) */
#define THEME_MENU_HEIGHT    20    /* menu bar height */
#define THEME_BUTTON_W       16    /* title-bar button width */
#define THEME_BUTTON_H       14    /* title-bar button height */
#define THEME_BUTTON_PAD      2    /* padding between buttons */

/* Minimum window outer dimensions */
#define THEME_MIN_W          80
#define THEME_MIN_H          40

/* Corner resize grab zone (pixels from corner edge) */
#define THEME_RESIZE_GRAB     6

/*==========================================================================
 * Color scheme
 *=========================================================================*/

#define THEME_DESKTOP_COLOR       COLOR_CYAN

/* Active window */
#define THEME_ACTIVE_TITLE_BG     COLOR_BLUE
#define THEME_ACTIVE_TITLE_FG     COLOR_WHITE
#define THEME_ACTIVE_BORDER       COLOR_LIGHT_GRAY

/* Inactive window */
#define THEME_INACTIVE_TITLE_BG   COLOR_DARK_GRAY
#define THEME_INACTIVE_TITLE_FG   COLOR_LIGHT_GRAY
#define THEME_INACTIVE_BORDER     COLOR_DARK_GRAY

/* 3D bevel for buttons and frames */
#define THEME_BEVEL_LIGHT         COLOR_WHITE
#define THEME_BEVEL_DARK          COLOR_DARK_GRAY
#define THEME_BUTTON_FACE         COLOR_LIGHT_GRAY

/* Client area default */
#define THEME_CLIENT_BG           COLOR_WHITE

/*==========================================================================
 * Hit-test zones (returned by theme_hit_test)
 *=========================================================================*/

#define HT_NOWHERE    0
#define HT_CLIENT     1
#define HT_TITLEBAR   2
#define HT_CLOSE      3
#define HT_MAXIMIZE   4
#define HT_MINIMIZE   5
#define HT_BORDER_L   6
#define HT_BORDER_R   7
#define HT_BORDER_T   8
#define HT_BORDER_B   9
#define HT_BORDER_TL 10
#define HT_BORDER_TR 11
#define HT_BORDER_BL 12
#define HT_BORDER_BR 13
#define HT_MENUBAR   14

/*==========================================================================
 * Inline geometry helpers
 *=========================================================================*/

/* Compute the client-area rect in client coordinates (origin 0,0)
 * given the outer frame rect and window flags. */
static inline rect_t theme_client_rect(const rect_t *frame, uint16_t flags) {
    rect_t r;
    r.x = 0;
    r.y = 0;
    r.w = frame->w - 2 * THEME_BORDER_WIDTH;
    r.h = frame->h - THEME_TITLE_HEIGHT - 2 * THEME_BORDER_WIDTH;
    if (flags & WF_MENUBAR) r.h -= THEME_MENU_HEIGHT;
    if (r.w < 0) r.w = 0;
    if (r.h < 0) r.h = 0;
    return r;
}

/* Client area origin in screen coordinates */
static inline point_t theme_client_origin(const rect_t *frame, uint16_t flags) {
    point_t p;
    p.x = frame->x + THEME_BORDER_WIDTH;
    p.y = frame->y + THEME_BORDER_WIDTH + THEME_TITLE_HEIGHT;
    if (flags & WF_MENUBAR) p.y += THEME_MENU_HEIGHT;
    return p;
}

/* Close button rect in screen coordinates */
static inline rect_t theme_close_btn_rect(const rect_t *frame) {
    rect_t r;
    r.w = THEME_BUTTON_W;
    r.h = THEME_BUTTON_H;
    r.x = frame->x + frame->w - THEME_BORDER_WIDTH - THEME_BUTTON_W -
          THEME_BUTTON_PAD;
    r.y = frame->y + THEME_BORDER_WIDTH + (THEME_TITLE_HEIGHT - THEME_BUTTON_H) / 2;
    return r;
}

/* Maximize button rect in screen coordinates */
static inline rect_t theme_max_btn_rect(const rect_t *frame) {
    rect_t r;
    r.w = THEME_BUTTON_W;
    r.h = THEME_BUTTON_H;
    r.x = frame->x + frame->w - THEME_BORDER_WIDTH -
          2 * (THEME_BUTTON_W + THEME_BUTTON_PAD);
    r.y = frame->y + THEME_BORDER_WIDTH + (THEME_TITLE_HEIGHT - THEME_BUTTON_H) / 2;
    return r;
}

/* Minimize button rect in screen coordinates */
static inline rect_t theme_min_btn_rect(const rect_t *frame) {
    rect_t r;
    r.w = THEME_BUTTON_W;
    r.h = THEME_BUTTON_H;
    r.x = frame->x + frame->w - THEME_BORDER_WIDTH -
          3 * (THEME_BUTTON_W + THEME_BUTTON_PAD);
    r.y = frame->y + THEME_BORDER_WIDTH + (THEME_TITLE_HEIGHT - THEME_BUTTON_H) / 2;
    return r;
}

/* Hit-test: given a screen-coordinate point and a window's outer frame,
 * return which zone the point falls in. */
static inline uint8_t theme_hit_test(const rect_t *frame, uint16_t flags,
                                      int16_t px, int16_t py) {
    /* Outside the frame entirely? */
    if (px < frame->x || px >= frame->x + frame->w ||
        py < frame->y || py >= frame->y + frame->h) {
        return HT_NOWHERE;
    }

    int16_t lx = px - frame->x;   /* local x within frame */
    int16_t ly = py - frame->y;   /* local y within frame */

    if (!(flags & WF_BORDER)) {
        return HT_CLIENT;
    }

    /* Corner resize zones — wider grab area, checked first so corners
     * take priority over the title bar and client area. */
    if (flags & WF_RESIZABLE) {
        int16_t cg = THEME_RESIZE_GRAB;
        bool cl = lx < cg;
        bool cr = lx >= frame->w - cg;
        bool ct = ly < cg;
        bool cb = ly >= frame->h - cg;

        if (ct && cl) return HT_BORDER_TL;
        if (ct && cr) return HT_BORDER_TR;
        if (cb && cl) return HT_BORDER_BL;
        if (cb && cr) return HT_BORDER_BR;
    }

    /* Title bar area (between top border and client) */
    if (ly >= THEME_BORDER_WIDTH &&
        ly < THEME_BORDER_WIDTH + THEME_TITLE_HEIGHT &&
        lx >= THEME_BORDER_WIDTH &&
        lx < frame->w - THEME_BORDER_WIDTH) {

        /* Check buttons (right side of title bar) */
        if (flags & WF_CLOSABLE) {
            rect_t cb = theme_close_btn_rect(frame);
            if (px >= cb.x && px < cb.x + cb.w &&
                py >= cb.y && py < cb.y + cb.h) {
                return HT_CLOSE;
            }
        }

        if (flags & WF_CLOSABLE) {
            rect_t mb = theme_max_btn_rect(frame);
            if (px >= mb.x && px < mb.x + mb.w &&
                py >= mb.y && py < mb.y + mb.h) {
                return HT_MAXIMIZE;
            }

            rect_t nb = theme_min_btn_rect(frame);
            if (px >= nb.x && px < nb.x + nb.w &&
                py >= nb.y && py < nb.y + nb.h) {
                return HT_MINIMIZE;
            }
        }

        return HT_TITLEBAR;
    }

    /* Menu bar zone (between title bar and client) */
    if (flags & WF_MENUBAR) {
        int16_t menu_top = THEME_BORDER_WIDTH + THEME_TITLE_HEIGHT;
        if (ly >= menu_top && ly < menu_top + THEME_MENU_HEIGHT &&
            lx >= THEME_BORDER_WIDTH &&
            lx < frame->w - THEME_BORDER_WIDTH) {
            return HT_MENUBAR;
        }
    }

    /* Client area */
    {
        int16_t client_top = THEME_BORDER_WIDTH + THEME_TITLE_HEIGHT;
        if (flags & WF_MENUBAR) client_top += THEME_MENU_HEIGHT;
        if (lx >= THEME_BORDER_WIDTH &&
            lx < frame->w - THEME_BORDER_WIDTH &&
            ly >= client_top &&
            ly < frame->h - THEME_BORDER_WIDTH) {
            return HT_CLIENT;
        }
    }

    /* Edge border zones */
    bool left   = lx < THEME_BORDER_WIDTH;
    bool right  = lx >= frame->w - THEME_BORDER_WIDTH;
    bool top    = ly < THEME_BORDER_WIDTH;
    bool bottom = ly >= frame->h - THEME_BORDER_WIDTH;

    if (left)   return HT_BORDER_L;
    if (right)  return HT_BORDER_R;
    if (top)    return HT_BORDER_T;
    if (bottom) return HT_BORDER_B;

    return HT_NOWHERE;
}

#endif /* WINDOW_THEME_H */
