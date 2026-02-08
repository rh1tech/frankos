#include "window_draw.h"
#include "window_theme.h"
#include "gfx.h"
#include "display.h"

/*==========================================================================
 * Drawing context — tracks current window's clip rect and origin
 *=========================================================================*/

static struct {
    int16_t ox, oy;     /* origin offset (screen coords of client 0,0) */
    int16_t cw, ch;     /* client area size */
    bool    active;      /* true between wd_begin/wd_end */
} draw_ctx;

/*==========================================================================
 * Context management
 *=========================================================================*/

void wd_begin(hwnd_t hwnd) {
    window_t *win = wm_get_window(hwnd);
    if (!win) return;

    if (win->flags & WF_BORDER) {
        point_t origin = theme_client_origin(&win->frame);
        rect_t client = theme_client_rect(&win->frame);
        draw_ctx.ox = origin.x;
        draw_ctx.oy = origin.y;
        draw_ctx.cw = client.w;
        draw_ctx.ch = client.h;
    } else {
        draw_ctx.ox = win->frame.x;
        draw_ctx.oy = win->frame.y;
        draw_ctx.cw = win->frame.w;
        draw_ctx.ch = win->frame.h;
    }
    draw_ctx.active = true;
}

void wd_end(void) {
    draw_ctx.active = false;
}

/*==========================================================================
 * Primitives — stub implementations
 *=========================================================================*/

void wd_pixel(int16_t x, int16_t y, uint8_t color) {
    (void)x; (void)y; (void)color;
    /* TODO: implement with clipping */
}

void wd_hline(int16_t x, int16_t y, int16_t w, uint8_t color) {
    (void)x; (void)y; (void)w; (void)color;
    /* TODO: implement */
}

void wd_vline(int16_t x, int16_t y, int16_t h, uint8_t color) {
    (void)x; (void)y; (void)h; (void)color;
    /* TODO: implement */
}

void wd_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    (void)x; (void)y; (void)w; (void)h; (void)color;
    /* TODO: implement */
}

void wd_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    (void)x; (void)y; (void)w; (void)h; (void)color;
    /* TODO: implement */
}

void wd_clear(uint8_t color) {
    (void)color;
    /* TODO: implement — fill entire client area */
}

void wd_char(int16_t x, int16_t y, char c, uint8_t fg, uint8_t bg) {
    (void)x; (void)y; (void)c; (void)fg; (void)bg;
    /* TODO: implement */
}

void wd_text(int16_t x, int16_t y, const char *str, uint8_t fg, uint8_t bg) {
    (void)x; (void)y; (void)str; (void)fg; (void)bg;
    /* TODO: implement */
}

void wd_text_transparent(int16_t x, int16_t y, const char *str, uint8_t fg) {
    (void)x; (void)y; (void)str; (void)fg;
    /* TODO: implement */
}

void wd_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
    (void)x0; (void)y0; (void)x1; (void)y1; (void)color;
    /* TODO: implement Bresenham */
}

void wd_bevel_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                    uint8_t light, uint8_t dark, uint8_t face) {
    (void)x; (void)y; (void)w; (void)h;
    (void)light; (void)dark; (void)face;
    /* TODO: implement 3D bevel */
}
