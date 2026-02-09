#include "gfx.h"
#include "display.h"
#include "font.h"

void gfx_hline(int x, int y, int w, uint8_t color) {
    for (int i = 0; i < w; i++)
        display_set_pixel(x + i, y, color);
}

void gfx_vline(int x, int y, int h, uint8_t color) {
    for (int i = 0; i < h; i++)
        display_set_pixel(x, y + i, color);
}

void gfx_fill_rect(int x, int y, int w, int h, uint8_t color) {
    for (int row = 0; row < h; row++)
        for (int col = 0; col < w; col++)
            display_set_pixel(x + col, y + row, color);
}

void gfx_rect(int x, int y, int w, int h, uint8_t color) {
    gfx_hline(x, y, w, color);
    gfx_hline(x, y + h - 1, w, color);
    gfx_vline(x, y, h, color);
    gfx_vline(x + w - 1, y, h, color);
}

void gfx_char(int x, int y, char c, uint8_t fg, uint8_t bg) {
    const uint8_t *glyph = font_get_glyph(c);
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            display_set_pixel(x + col, y + row,
                              (bits & (1 << col)) ? fg : bg);
        }
    }
}

void gfx_text(int x, int y, const char *str, uint8_t fg, uint8_t bg) {
    while (*str) {
        gfx_char(x, y, *str, fg, bg);
        x += FONT_WIDTH;
        str++;
    }
}

void gfx_fill_rect_clipped(int x, int y, int w, int h, uint8_t color,
                            int cx, int cy, int cw, int ch) {
    /* Intersect (x,y,w,h) with clip rect (cx,cy,cw,ch) */
    int x0 = x < cx ? cx : x;
    int y0 = y < cy ? cy : y;
    int x1 = (x + w) < (cx + cw) ? (x + w) : (cx + cw);
    int y1 = (y + h) < (cy + ch) ? (y + h) : (cy + ch);

    if (x0 >= x1 || y0 >= y1) return;

    for (int row = y0; row < y1; row++)
        for (int col = x0; col < x1; col++)
            display_set_pixel(col, row, color);
}

void gfx_char_clipped(int x, int y, char c, uint8_t fg, uint8_t bg,
                       int cx, int cy, int cw, int ch) {
    const uint8_t *glyph = font_get_glyph(c);
    int cx1 = cx + cw;
    int cy1 = cy + ch;

    for (int row = 0; row < FONT_HEIGHT; row++) {
        int py = y + row;
        if (py < cy || py >= cy1) continue;
        uint8_t bits = glyph[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            int px = x + col;
            if (px < cx || px >= cx1) continue;
            display_set_pixel(px, py, (bits & (1 << col)) ? fg : bg);
        }
    }
}

void gfx_text_clipped(int x, int y, const char *str, uint8_t fg, uint8_t bg,
                       int cx, int cy, int cw, int ch) {
    while (*str) {
        /* Skip chars entirely outside clip rect */
        if (x + FONT_WIDTH > cx && x < cx + cw)
            gfx_char_clipped(x, y, *str, fg, bg, cx, cy, cw, ch);
        x += FONT_WIDTH;
        str++;
    }
}
