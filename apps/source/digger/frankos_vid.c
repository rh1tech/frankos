/*
 * frankos_vid.c - FRANK OS Video Backend for Digger
 *
 * Replaces rp2350_vid.c. Renders to an internal 320x240 framebuffer
 * (4-bit nibble-packed). The paint callback copies it to the FRANK OS
 * window framebuffer with CGA-to-CGA16 palette translation.
 *
 * Copyright (c) 2026 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "m-os-api.h"
#include "frankos-app.h"

#undef switch
#undef inline
#undef __force_inline
#undef abs

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "def.h"
#include "draw_api.h"
#include "alpha.h"
#include "frankos_app.h"

/* CGA sprite table from cgagrafx.c */
extern const uint8_t *cgatable[];

/* CGA alpha font from alpha.c - 2bpp packed, 3 bytes/row, 12 rows */
extern const uint8_t * const ascii2cga[];

/* Global app state */
extern digger_app_t *g_app;

/*
 * CGA Palette definitions -> FRANK OS COLOR_* mapping
 *
 * CGA Palette 0 (pal=0): Black, Green, Red, Brown
 * CGA Palette 1 (pal=1): Black, Cyan, Magenta, White
 * Intensity variants double brightness.
 */
static const uint8_t cga_pal0[]  = {COLOR_BLACK, COLOR_GREEN,       COLOR_RED,         COLOR_BROWN};
static const uint8_t cga_pal0i[] = {COLOR_BLACK, COLOR_LIGHT_GREEN, COLOR_LIGHT_RED,   COLOR_YELLOW};
static const uint8_t cga_pal1[]  = {COLOR_BLACK, COLOR_CYAN,        COLOR_MAGENTA,     COLOR_LIGHT_GRAY};
static const uint8_t cga_pal1i[] = {COLOR_BLACK, COLOR_LIGHT_CYAN,  COLOR_LIGHT_MAGENTA, COLOR_WHITE};

static int16_t current_pal = 0;
static int16_t current_inten = 0;

/*
 * Framebuffer access helpers.
 * Internal framebuffer is 4-bit per pixel, nibble-packed.
 * Low nibble = left pixel (even x), high nibble = right pixel (odd x).
 * Row stride = 320/2 = 160 bytes.
 */
/*
 * FRANK OS nibble order: high nibble = left (even x), low nibble = right (odd x).
 */
static inline void fb_set_pixel(int x, int y, uint8_t color) {
    int fb_y = y + DIGGER_Y_OFFSET;
    if (fb_y < 0 || fb_y >= DIGGER_FB_H || x < 0 || x >= DIGGER_FB_W)
        return;
    uint8_t *fb = g_app->framebuffer;
    int idx = fb_y * DIGGER_FB_STRIDE + (x >> 1);
    if (x & 1)
        fb[idx] = (fb[idx] & 0xF0) | (color & 0x0F);
    else
        fb[idx] = (fb[idx] & 0x0F) | ((color & 0x0F) << 4);
}

static inline uint8_t fb_get_pixel(int x, int y) {
    int fb_y = y + DIGGER_Y_OFFSET;
    if (fb_y < 0 || fb_y >= DIGGER_FB_H || x < 0 || x >= DIGGER_FB_W)
        return 0;
    uint8_t *fb = g_app->framebuffer;
    int idx = fb_y * DIGGER_FB_STRIDE + (x >> 1);
    if (x & 1)
        return fb[idx] & 0x0F;
    else
        return (fb[idx] >> 4) & 0x0F;
}

/*
 * Update the CGA-to-FRANK palette lookup table.
 */
static void apply_palette(void) {
    const uint8_t *pal;
    if (current_pal == 0)
        pal = current_inten ? cga_pal0i : cga_pal0;
    else
        pal = current_inten ? cga_pal1i : cga_pal1;

    for (int i = 0; i < 4; i++)
        g_app->cga_to_color[i] = pal[i];
}

/*
 * cgainit - Initialize video (allocate framebuffer done in main)
 */
void cgainit(void) {
    apply_palette();
}

/*
 * cgaclear - Clear entire internal framebuffer to black
 */
void cgaclear(void) {
    memset(g_app->framebuffer, 0, DIGGER_FB_STRIDE * DIGGER_FB_H);
}

/*
 * cgapal - Set CGA palette (0 or 1)
 */
void cgapal(int16_t pal) {
    current_pal = pal;
    apply_palette();
}

/*
 * cgainten - Switch between normal/high intensity palette
 */
void cgainten(int16_t inten) {
    current_inten = inten;
    apply_palette();
}

/*
 * cgaputi - Copy raw 4-bit packed pixels from buffer p to framebuffer
 *
 * Buffer format: 4-bit nibble-packed, same as framebuffer.
 * w = width in "sprite units" (w*4 = pixel width), h = height in pixels.
 * Buffer stores (w*4/2) = w*2 bytes per row.
 */
void cgaputi(int16_t x, int16_t y, uint8_t *p, int16_t w, int16_t h) {
    int pixel_w = w * 4;
    int buf_stride = pixel_w / 2;
    uint8_t *fb = g_app->framebuffer;

    for (int row = 0; row < h; row++) {
        int fb_y = (y + row) + DIGGER_Y_OFFSET;
        if (fb_y < 0 || fb_y >= DIGGER_FB_H)
            continue;
        int fb_offset = fb_y * DIGGER_FB_STRIDE + (x >> 1);
        int buf_offset = row * buf_stride;
        memcpy(&fb[fb_offset], &p[buf_offset], buf_stride);
    }
}

/*
 * cgageti - Copy 4-bit packed pixels from framebuffer to buffer p
 */
void cgageti(int16_t x, int16_t y, uint8_t *p, int16_t w, int16_t h) {
    int pixel_w = w * 4;
    int buf_stride = pixel_w / 2;
    uint8_t *fb = g_app->framebuffer;

    for (int row = 0; row < h; row++) {
        int fb_y = (y + row) + DIGGER_Y_OFFSET;
        if (fb_y < 0 || fb_y >= DIGGER_FB_H)
            continue;
        int fb_offset = fb_y * DIGGER_FB_STRIDE + (x >> 1);
        int buf_offset = row * buf_stride;
        memcpy(&p[buf_offset], &fb[fb_offset], buf_stride);
    }
}

/*
 * cgaputim - Draw CGA sprite with mask (transparency)
 *
 * CGA sprite data: 2 bits per pixel, 4 pixels per byte.
 * cgatable[ch*2] = sprite data, cgatable[ch*2+1] = mask.
 * w = bytes per row (each byte = 4 pixels), h = rows.
 *
 * For each pixel: result = (screen & mask) | sprite
 * Mask bit 1 = transparent (keep screen), mask bit 0 = opaque (use sprite)
 */
void cgaputim(int16_t x, int16_t y, int16_t ch, int16_t w, int16_t h) {
    const uint8_t *sprite = cgatable[ch * 2];
    const uint8_t *mask = cgatable[ch * 2 + 1];

    for (int row = 0; row < h; row++) {
        int px = x;
        for (int col = 0; col < w; col++) {
            uint8_t sbyte = sprite[row * w + col];
            uint8_t mbyte = mask[row * w + col];

            for (int bit = 6; bit >= 0; bit -= 2) {
                uint8_t spix = (sbyte >> bit) & 0x03;
                uint8_t mpix = (mbyte >> bit) & 0x03;

                if (mpix != 0x03) {
                    uint8_t screen_pix = fb_get_pixel(px, y + row);
                    uint8_t result = (screen_pix & mpix) | spix;
                    fb_set_pixel(px, y + row, result);
                } else if (spix != 0) {
                    fb_set_pixel(px, y + row, spix);
                }
                px++;
            }
        }
    }
}

/*
 * cgagetpix - Read pixel collision data from framebuffer
 *
 * Returns CGA-format byte: 4 pixels at 2 bits each, MSB first.
 */
int16_t cgagetpix(int16_t x, int16_t y) {
    int16_t rval = 0;

    if (x < 0 || x > 319 || y < 0 || y > 199)
        return 0xff;

    for (int xi = 0; xi < 4; xi++) {
        uint8_t pix = fb_get_pixel(x + xi, y) & 0x03;
        rval |= pix << (6 - xi * 2);
    }

    return rval;
}

/*
 * cgawrite - Draw text character
 *
 * CGA alpha font: 3 bytes per row, 12 rows.
 * Each byte = 4 pixels at 2 bits each = 12 pixels wide.
 */
void cgawrite(int16_t x, int16_t y, int16_t ch, int16_t c) {
    const uint8_t *font;

    if (!isvalchar(ch))
        return;

    font = ascii2cga[ch - 32];
    if (font == NULL)
        return;

    for (int row = 0; row < 12; row++) {
        int px = x;
        for (int col = 0; col < 3; col++) {
            uint8_t byte = font[row * 3 + col];
            for (int bit = 6; bit >= 0; bit -= 2) {
                uint8_t pix = (byte >> bit) & 0x03;
                fb_set_pixel(px, y + row, pix ? c : 0);
                px++;
            }
        }
    }
}

/*
 * cgatitle - Draw title screen border
 */
void cgatitle(void) {
    int x, y, t;

    cgaclear();

    #define BRD_L   4
    #define BRD_R   317
    #define BRD_T   16
    #define BRD_B   185
    #define BRD_W   3
    #define BRD_DIV 160

    for (x = BRD_L; x <= BRD_R; x++)
        for (t = 0; t < BRD_W; t++) {
            fb_set_pixel(x, BRD_T + t, 2);
            fb_set_pixel(x, BRD_B - t, 2);
        }
    for (y = BRD_T; y <= BRD_B; y++)
        for (t = 0; t < BRD_W; t++) {
            fb_set_pixel(BRD_L + t, y, 2);
            fb_set_pixel(BRD_R - t, y, 2);
        }
    for (y = BRD_T; y <= BRD_B; y++)
        for (t = 0; t < BRD_W; t++)
            fb_set_pixel(BRD_DIV - 1 + t, y, 2);
}

/*
 * doscreenupdate - Request FRANK OS to repaint the window
 */
void doscreenupdate(void) {
    if (g_app && g_app->app_hwnd != HWND_NULL)
        wm_invalidate(g_app->app_hwnd);
}

/*
 * graphicsoff - No-op on FRANK OS
 */
void graphicsoff(void) {
}

/*
 * gretrace - No-op on FRANK OS
 */
void gretrace(void) {
}

/*
 * digger_paint - Paint callback (runs on WM/compositor task)
 *
 * Copies the internal 320x240 framebuffer to the FRANK OS window,
 * translating CGA palette indices (0-3) to FRANK OS COLOR_* values.
 */
void digger_paint(hwnd_t hwnd) {
    wd_begin(hwnd);

    bool fs = wm_is_fullscreen(hwnd);

    int16_t stride;
    uint8_t *dst;
    if (fs) {
        /* In fullscreen (640×480), center 640×400 with 40-row black bars */
        int bar = 40;
        /* Fill top and bottom bars black */
        wd_fill_rect(0, 0, 640, bar, COLOR_BLACK);
        wd_fill_rect(0, bar + DIGGER_HEIGHT * 2, 640, bar, COLOR_BLACK);
        dst = wd_fb_ptr(0, bar, &stride);
    } else {
        dst = wd_fb_ptr(0, 0, &stride);
    }
    if (!dst || !g_app || !g_app->framebuffer) {
        wd_end();
        return;
    }

    const uint8_t *src = g_app->framebuffer;
    const uint8_t *lut = g_app->cga_to_color;

    /* Clamp rendering to visible client area to prevent scanline overflow. */
    int16_t clip_w, clip_h;
    wd_get_clip_size(&clip_w, &clip_h);

    if (!fs) {
        /* ---- Normal mode: 1x rendering ---- */
        int max_bytes = clip_w / 2;
        if (max_bytes > DIGGER_FB_STRIDE) max_bytes = DIGGER_FB_STRIDE;
        int max_rows = clip_h;
        if (max_rows > DIGGER_HEIGHT) max_rows = DIGGER_HEIGHT;

        for (int y = 0; y < max_rows; y++) {
            const uint8_t *srow = &src[(y + DIGGER_Y_OFFSET) * DIGGER_FB_STRIDE];
            uint8_t *drow = &dst[y * stride];
            for (int bx = 0; bx < max_bytes; bx++) {
                uint8_t sb = srow[bx];
                uint8_t left  = lut[(sb >> 4) & 0x0F];
                uint8_t right = lut[sb & 0x0F];
                drow[bx] = (left << 4) | right;
            }
        }
    } else {
        /* ---- Fullscreen mode: 2x rendering ---- */
        int max_bytes = clip_w / 4;   /* each src byte → 2 dst bytes at 2x */
        if (max_bytes > DIGGER_FB_STRIDE) max_bytes = DIGGER_FB_STRIDE;
        int max_rows = (clip_h - 40) / 2;  /* subtract top bar, halve for 2x */
        if (max_rows > DIGGER_HEIGHT) max_rows = DIGGER_HEIGHT;

        for (int y = 0; y < max_rows; y++) {
            const uint8_t *srow = &src[(y + DIGGER_Y_OFFSET) * DIGGER_FB_STRIDE];
            uint8_t *row0 = &dst[(y * 2) * stride];
            uint8_t *row1 = row0 + stride;

            for (int bx = 0; bx < max_bytes; bx++) {
                uint8_t sb = srow[bx];
                uint8_t left  = lut[(sb >> 4) & 0x0F];
                uint8_t right = lut[sb & 0x0F];
                /* Each source pixel (nibble) → 2 destination pixels:
                 * left nibble: LL, right nibble: RR */
                uint8_t b0 = (left << 4) | left;
                uint8_t b1 = (right << 4) | right;
                row0[bx * 2]     = b0;
                row0[bx * 2 + 1] = b1;
                row1[bx * 2]     = b0;
                row1[bx * 2 + 1] = b1;
            }
        }
    }

    wd_end();
}
