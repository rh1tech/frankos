#ifndef FONT_H
#define FONT_H

#include <stdint.h>

/* 8x16 fixed-width bitmap font, ASCII 32-126 (95 glyphs).
 * Each glyph is 16 bytes — one byte per row, MSB = leftmost pixel.
 * Total: 95 * 16 = 1520 bytes, stored in flash (.rodata). */

#define FONT_WIDTH   8
#define FONT_HEIGHT  16
#define FONT_FIRST   32   /* space */
#define FONT_LAST    126  /* tilde */
#define FONT_GLYPHS  (FONT_LAST - FONT_FIRST + 1)

/* Glyph data array — defined in font.c */
extern const uint8_t font_glyphs[FONT_GLYPHS][FONT_HEIGHT];

/* Return pointer to glyph row data, or NULL for out-of-range chars */
static inline const uint8_t *font_get_glyph(char c) {
    if (c < FONT_FIRST || c > FONT_LAST) return (const uint8_t *)0;
    return font_glyphs[c - FONT_FIRST];
}

#endif /* FONT_H */
