/*
 * FRANK OS
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FONT_H
#define FONT_H

#include <stdint.h>

/* 8x8 fixed-width bitmap font, CP866 (256 glyphs).
 * Each glyph is 8 bytes — one byte per row, MSB = leftmost pixel.
 * Total: 256 * 8 = 2048 bytes, stored in flash (.rodata). */
#define FONT8x8_WIDTH   8
#define FONT8x8_HEIGHT  8

extern const uint8_t font_8x8[2048];

static inline const uint8_t *font8x8_get_glyph(uint8_t c) {
    return &font_8x8[c * 8];
}

/* 8x16 fixed-width bitmap font, CP866 (256 glyphs).
 * Each glyph is 16 bytes — one byte per row, MSB = leftmost pixel.
 * Total: 256 * 16 = 4096 bytes, stored in flash (.rodata). */
#define FONT8x16_WIDTH  8
#define FONT8x16_HEIGHT 16

extern const uint8_t font_8x16[4096];

static inline const uint8_t *font8x16_get_glyph(uint8_t c) {
    return &font_8x16[c * 16];
}

/* Default font (8x8) — used by gfx.c and window title/text drawing */
#define FONT_WIDTH   8
#define FONT_HEIGHT  8

static inline const uint8_t *font_get_glyph(char c) {
    uint8_t u = (uint8_t)c;
    if (u >= 'a' && u <= 'z') u -= 32;
    return &font_8x8[u * 8];
}

/* 8x12 fixed-width UI font — Tamzen 6x12 Regular weight.
 * Full ASCII with lowercase. Used for window chrome, menus, taskbar.
 * Each glyph is 12 bytes — one byte per row, MSB = leftmost pixel.
 * Total: 256 * 12 = 3072 bytes, stored in flash (.rodata). */
#define FONT_UI_WIDTH   6
#define FONT_UI_HEIGHT  12

extern const uint8_t font_ui_8x12[3072];

static inline const uint8_t *font_ui_get_glyph(uint8_t c) {
    return &font_ui_8x12[c * 12];
}

/* 8x12 fixed-width UI font — Tamzen 6x12 Bold weight.
 * Separate bitmap data (not runtime-emboldened).
 * Used for window title bars and the Start button. */
extern const uint8_t font_ui_bold_8x12[3072];

static inline const uint8_t *font_ui_bold_get_glyph(uint8_t c) {
    return &font_ui_bold_8x12[c * 12];
}

#endif /* FONT_H */
