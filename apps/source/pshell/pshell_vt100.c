/*
 * pshell_vt100.c — VT100 terminal emulator for pshell on FRANK OS
 *
 * State machine processes output characters and translates VT100 escape
 * sequences into terminal buffer operations.  The buffer is rendered to
 * a FRANK OS window framebuffer using wd_fb_ptr() with an 8×16 font.
 *
 * Copyright (c) 2026 Mikhail Matveev <xtreme@rh1.tech>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "m-os-api.h"
#include "frankos-app.h"
#include "pshell_vt100.h"
#include "font.h"

#include <string.h>
#include <stdarg.h>

/* ═════════════════════════════════════════════════════════════════════════
 * ANSI → CGA/EGA color mapping
 * ═════════════════════════════════════════════════════════════════════════ */

/* ANSI color index (0-7) → CGA color index (used in attr byte) */
static const uint8_t ansi_to_cga[8] = {
    COLOR_BLACK,        /* 0 */
    COLOR_RED,          /* 1 */
    COLOR_GREEN,        /* 2 */
    COLOR_BROWN,        /* 3  (ANSI yellow → CGA brown at non-bright) */
    COLOR_BLUE,         /* 4 */
    COLOR_MAGENTA,      /* 5 */
    COLOR_CYAN,         /* 6 */
    COLOR_LIGHT_GRAY    /* 7 */
};

/* ═════════════════════════════════════════════════════════════════════════
 * Terminal state
 * ═════════════════════════════════════════════════════════════════════════ */

/* VT100 parser states */
enum { ST_NORMAL, ST_ESC_START, ST_CSI_PARAM, ST_CSI_INTER };

/* CSI parameter accumulation */
#define MAX_CSI_PARAMS  8

/* Text buffer: 2 bytes per cell [char][attr], row-major */
static uint8_t *textbuf;
static int      tb_cols, tb_rows;
static int      cursor_col, cursor_row;
static uint8_t  cur_fg, cur_bg;
static bool     bold_on, reverse_on;

/* VT100 parser state */
static int      vt_state;
static int      csi_params[MAX_CSI_PARAMS];
static int      csi_nparam;
static int      csi_cur_param;

/* Cursor blink state */
static volatile bool cursor_visible;

/* Window handle for invalidation */
static hwnd_t   g_vt_hwnd;

/* Shadow buffer for dirty-cell paint optimisation */
static uint8_t *shadow_buf;
static uint8_t *last_fb_ptr;

/* ── Input ring buffer ─────────────────────────────────────────────────── */
static volatile uint8_t inbuf[VT100_INBUF_SIZE];
static volatile int     in_head, in_tail;
static SemaphoreHandle_t in_sem;  /* counting semaphore: count = available bytes */

/* ═════════════════════════════════════════════════════════════════════════
 * Helpers
 * ═════════════════════════════════════════════════════════════════════════ */

/* Offset into textbuf for (row, col) */
#define TB_OFF(r, c)  (((r) * tb_cols + (c)) * 2)

static inline uint8_t make_attr(void) {
    uint8_t fg = cur_fg;
    uint8_t bg = cur_bg;
    if (bold_on) fg |= 8;
    if (reverse_on) { uint8_t t = fg; fg = bg; bg = t; }
    return (bg << 4) | (fg & 0x0F);
}

static void tb_put_char(int row, int col, uint8_t ch, uint8_t attr) {
    if (row < 0 || row >= tb_rows || col < 0 || col >= tb_cols) return;
    int off = TB_OFF(row, col);
    textbuf[off]     = ch;
    textbuf[off + 1] = attr;
}

static void scroll_up(void) {
    /* Move rows 1..last to 0..last-1 */
    int row_bytes = tb_cols * 2;
    memmove(textbuf, textbuf + row_bytes, (tb_rows - 1) * row_bytes);
    /* Clear last row */
    uint8_t attr = make_attr();
    int last_off = (tb_rows - 1) * row_bytes;
    for (int c = 0; c < tb_cols; c++) {
        textbuf[last_off + c * 2]     = ' ';
        textbuf[last_off + c * 2 + 1] = attr;
    }
}

static void invalidate(void) {
    if (g_vt_hwnd != 0)
        wm_invalidate(g_vt_hwnd);
}

/* ═════════════════════════════════════════════════════════════════════════
 * CSI command execution
 * ═════════════════════════════════════════════════════════════════════════ */

static void csi_execute(char cmd) {
    int p0 = (csi_nparam > 0) ? csi_params[0] : 0;
    int p1 = (csi_nparam > 1) ? csi_params[1] : 0;
    uint8_t attr;
    int off;

    if (cmd == 'H' || cmd == 'f') {
        /* CUP — cursor position (1-based params, default 1;1) */
        int row = (p0 > 0) ? p0 - 1 : 0;
        int col = (p1 > 0) ? p1 - 1 : 0;
        if (row >= tb_rows) row = tb_rows - 1;
        if (col >= tb_cols) col = tb_cols - 1;
        cursor_row = row;
        cursor_col = col;
    }
    else if (cmd == 'A') {
        /* CUU — cursor up */
        int n = (p0 > 0) ? p0 : 1;
        cursor_row -= n;
        if (cursor_row < 0) cursor_row = 0;
    }
    else if (cmd == 'B') {
        /* CUD — cursor down */
        int n = (p0 > 0) ? p0 : 1;
        cursor_row += n;
        if (cursor_row >= tb_rows) cursor_row = tb_rows - 1;
    }
    else if (cmd == 'C') {
        /* CUF — cursor forward */
        int n = (p0 > 0) ? p0 : 1;
        cursor_col += n;
        if (cursor_col >= tb_cols) cursor_col = tb_cols - 1;
    }
    else if (cmd == 'D') {
        /* CUB — cursor back */
        int n = (p0 > 0) ? p0 : 1;
        cursor_col -= n;
        if (cursor_col < 0) cursor_col = 0;
    }
    else if (cmd == 'J') {
        /* ED — erase in display */
        attr = make_attr();
        if (p0 == 0) {
            /* Clear from cursor to end of screen */
            for (int c = cursor_col; c < tb_cols; c++)
                tb_put_char(cursor_row, c, ' ', attr);
            for (int r = cursor_row + 1; r < tb_rows; r++)
                for (int c = 0; c < tb_cols; c++)
                    tb_put_char(r, c, ' ', attr);
        } else if (p0 == 1) {
            /* Clear from start to cursor */
            for (int r = 0; r < cursor_row; r++)
                for (int c = 0; c < tb_cols; c++)
                    tb_put_char(r, c, ' ', attr);
            for (int c = 0; c <= cursor_col; c++)
                tb_put_char(cursor_row, c, ' ', attr);
        } else if (p0 == 2) {
            /* Clear entire screen */
            for (int r = 0; r < tb_rows; r++)
                for (int c = 0; c < tb_cols; c++)
                    tb_put_char(r, c, ' ', attr);
        }
    }
    else if (cmd == 'K') {
        /* EL — erase in line */
        attr = make_attr();
        if (p0 == 0) {
            /* Clear from cursor to end of line */
            for (int c = cursor_col; c < tb_cols; c++)
                tb_put_char(cursor_row, c, ' ', attr);
        } else if (p0 == 1) {
            /* Clear from start of line to cursor */
            for (int c = 0; c <= cursor_col; c++)
                tb_put_char(cursor_row, c, ' ', attr);
        } else if (p0 == 2) {
            /* Clear entire line */
            for (int c = 0; c < tb_cols; c++)
                tb_put_char(cursor_row, c, ' ', attr);
        }
    }
    else if (cmd == 'm') {
        /* SGR — select graphic rendition */
        if (csi_nparam == 0) {
            /* ESC[m = reset */
            cur_fg = COLOR_LIGHT_GRAY;
            cur_bg = COLOR_BLACK;
            bold_on = false;
            reverse_on = false;
            return;
        }
        for (int i = 0; i < csi_nparam; i++) {
            int p = csi_params[i];
            if (p == 0) {
                cur_fg = COLOR_LIGHT_GRAY;
                cur_bg = COLOR_BLACK;
                bold_on = false;
                reverse_on = false;
            } else if (p == 1) {
                bold_on = true;
            } else if (p == 5) {
                /* Blink → map to bright background (or ignore) */
            } else if (p == 7) {
                reverse_on = true;
            } else if (p == 22) {
                bold_on = false;
            } else if (p == 27) {
                reverse_on = false;
            } else if (p >= 30 && p <= 37) {
                cur_fg = ansi_to_cga[p - 30];
            } else if (p >= 40 && p <= 47) {
                cur_bg = ansi_to_cga[p - 40];
            } else if (p >= 90 && p <= 97) {
                /* Bright foreground */
                cur_fg = ansi_to_cga[p - 90] | 8;
            } else if (p >= 100 && p <= 107) {
                /* Bright background */
                cur_bg = ansi_to_cga[p - 100] | 8;
            }
        }
    }
    else if (cmd == 'n') {
        /* DSR — device status report */
        if (p0 == 6) {
            /* Cursor position report: push ESC[row;colR into input */
            char resp[24];
            int len = snprintf(resp, sizeof(resp), "\033[%d;%dR",
                               cursor_row + 1, cursor_col + 1);
            for (int i = 0; i < len; i++)
                vt100_input_push(resp[i]);
        }
    }
    else if (cmd == 'L') {
        /* IL — insert line(s) */
        int n = (p0 > 0) ? p0 : 1;
        if (cursor_row + n < tb_rows) {
            int row_bytes = tb_cols * 2;
            memmove(textbuf + (cursor_row + n) * row_bytes,
                    textbuf + cursor_row * row_bytes,
                    (tb_rows - cursor_row - n) * row_bytes);
        }
        attr = make_attr();
        for (int i = 0; i < n && cursor_row + i < tb_rows; i++)
            for (int c = 0; c < tb_cols; c++)
                tb_put_char(cursor_row + i, c, ' ', attr);
    }
    else if (cmd == 'M') {
        /* DL — delete line(s) */
        int n = (p0 > 0) ? p0 : 1;
        if (cursor_row + n < tb_rows) {
            int row_bytes = tb_cols * 2;
            memmove(textbuf + cursor_row * row_bytes,
                    textbuf + (cursor_row + n) * row_bytes,
                    (tb_rows - cursor_row - n) * row_bytes);
        }
        attr = make_attr();
        for (int r = tb_rows - n; r < tb_rows; r++)
            for (int c = 0; c < tb_cols; c++)
                tb_put_char(r, c, ' ', attr);
    }
    else if (cmd == 'P') {
        /* DCH — delete character(s) */
        int n = (p0 > 0) ? p0 : 1;
        off = TB_OFF(cursor_row, 0);
        if (cursor_col + n < tb_cols) {
            memmove(textbuf + off + cursor_col * 2,
                    textbuf + off + (cursor_col + n) * 2,
                    (tb_cols - cursor_col - n) * 2);
        }
        attr = make_attr();
        for (int c = tb_cols - n; c < tb_cols; c++)
            tb_put_char(cursor_row, c, ' ', attr);
    }
    else if (cmd == '@') {
        /* ICH — insert character(s) */
        int n = (p0 > 0) ? p0 : 1;
        off = TB_OFF(cursor_row, 0);
        if (cursor_col + n < tb_cols) {
            memmove(textbuf + off + (cursor_col + n) * 2,
                    textbuf + off + cursor_col * 2,
                    (tb_cols - cursor_col - n) * 2);
        }
        attr = make_attr();
        for (int i = 0; i < n && cursor_col + i < tb_cols; i++)
            tb_put_char(cursor_row, cursor_col + i, ' ', attr);
    }
}

/* ═════════════════════════════════════════════════════════════════════════
 * VT100 state machine — processes one character at a time
 * ═════════════════════════════════════════════════════════════════════════ */

void vt100_putc(char c) {
    uint8_t uc = (uint8_t)c;

    if (vt_state == ST_NORMAL) {
        if (uc == '\033') {
            vt_state = ST_ESC_START;
        } else if (uc == '\n') {
            cursor_col = 0;
            cursor_row++;
            if (cursor_row >= tb_rows) {
                scroll_up();
                cursor_row = tb_rows - 1;
            }
            invalidate();
        } else if (uc == '\r') {
            cursor_col = 0;
        } else if (uc == '\b') {
            if (cursor_col > 0)
                cursor_col--;
        } else if (uc == '\t') {
            int next = (cursor_col + 8) & ~7;
            if (next > tb_cols) next = tb_cols;
            while (cursor_col < next) {
                tb_put_char(cursor_row, cursor_col, ' ', make_attr());
                cursor_col++;
            }
            if (cursor_col >= tb_cols) cursor_col = tb_cols - 1;
            invalidate();
        } else if (uc == '\007') {
            /* BEL — ignore */
        } else if (uc >= 0x20) {
            /* Printable character */
            if (cursor_col >= tb_cols) {
                /* Wrap to next line */
                cursor_col = 0;
                cursor_row++;
                if (cursor_row >= tb_rows) {
                    scroll_up();
                    cursor_row = tb_rows - 1;
                }
            }
            tb_put_char(cursor_row, cursor_col, uc, make_attr());
            cursor_col++;
            invalidate();
        }
    }
    else if (vt_state == ST_ESC_START) {
        if (uc == '[') {
            /* CSI introducer */
            vt_state = ST_CSI_PARAM;
            csi_nparam = 0;
            csi_cur_param = 0;
            memset(csi_params, 0, sizeof(csi_params));
        } else {
            /* Unknown ESC sequence — ignore and return to normal */
            vt_state = ST_NORMAL;
        }
    }
    else if (vt_state == ST_CSI_PARAM) {
        if (uc >= '0' && uc <= '9') {
            csi_cur_param = csi_cur_param * 10 + (uc - '0');
        } else if (uc == ';') {
            if (csi_nparam < MAX_CSI_PARAMS)
                csi_params[csi_nparam++] = csi_cur_param;
            csi_cur_param = 0;
        } else if (uc >= 0x20 && uc <= 0x2F) {
            /* Intermediate bytes — transition to CSI_INTER */
            vt_state = ST_CSI_INTER;
        } else if ((uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z') || uc == '@') {
            /* Final byte — push last param and execute */
            if (csi_nparam < MAX_CSI_PARAMS)
                csi_params[csi_nparam++] = csi_cur_param;
            csi_execute((char)uc);
            vt_state = ST_NORMAL;
            invalidate();
        } else {
            vt_state = ST_NORMAL;
        }
    }
    else if (vt_state == ST_CSI_INTER) {
        /* Consuming intermediate bytes; wait for final byte */
        if ((uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z') || uc == '@') {
            /* Push last param and execute (though most intermediates are ignored) */
            if (csi_nparam < MAX_CSI_PARAMS)
                csi_params[csi_nparam++] = csi_cur_param;
            csi_execute((char)uc);
            vt_state = ST_NORMAL;
            invalidate();
        }
        /* Otherwise keep consuming */
    }
}

void vt100_puts_nl(const char *s) {
    while (*s)
        vt100_putc(*s++);
    vt100_putc('\n');
}

int vt100_printf(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    for (int i = 0; i < n && buf[i]; i++)
        vt100_putc(buf[i]);
    return n;
}

/* ═════════════════════════════════════════════════════════════════════════
 * Input ring buffer
 * ═════════════════════════════════════════════════════════════════════════ */

void vt100_input_push(int c) {
    int next = (in_head + 1) % VT100_INBUF_SIZE;
    if (next != in_tail) {
        inbuf[in_head] = (uint8_t)(c & 0xFF);
        in_head = next;
    }
    if (in_sem)
        xSemaphoreGive(in_sem);
}

void vt100_input_push_str(const char *s) {
    while (*s)
        vt100_input_push((unsigned char)*s++);
}

int vt100_getch(void) {
    /* Block until a character is available */
    while (in_tail == in_head) {
        if (in_sem)
            xSemaphoreTake(in_sem, pdMS_TO_TICKS(100));
        else
            vTaskDelay(1);
    }
    int c = inbuf[in_tail];
    in_tail = (in_tail + 1) % VT100_INBUF_SIZE;
    return c;
}

int vt100_getch_timeout(int us) {
    /* Check if data available */
    if (in_tail != in_head) {
        int c = inbuf[in_tail];
        in_tail = (in_tail + 1) % VT100_INBUF_SIZE;
        return c;
    }
    /* Wait with timeout */
    int ticks = us / 1000;  /* microseconds to ms */
    if (ticks < 1) ticks = 1;
    ticks = pdMS_TO_TICKS(ticks);
    if (ticks < 1) ticks = 1;
    if (in_sem)
        xSemaphoreTake(in_sem, ticks);
    else
        vTaskDelay(ticks);
    if (in_tail != in_head) {
        int c = inbuf[in_tail];
        in_tail = (in_tail + 1) % VT100_INBUF_SIZE;
        return c;
    }
    return -1; /* PICO_ERROR_TIMEOUT */
}

void vt100_ungetc(int c) {
    /* Push character back to head of input buffer */
    int prev = (in_tail - 1 + VT100_INBUF_SIZE) % VT100_INBUF_SIZE;
    if (prev != in_head) {
        in_tail = prev;
        inbuf[in_tail] = (uint8_t)(c & 0xFF);
    }
}

/* ═════════════════════════════════════════════════════════════════════════
 * Initialisation & lifecycle
 * ═════════════════════════════════════════════════════════════════════════ */

void vt100_init(int cols, int rows) {
    if (cols > VT100_MAX_COLS) cols = VT100_MAX_COLS;
    if (rows > VT100_MAX_ROWS) rows = VT100_MAX_ROWS;
    tb_cols = cols;
    tb_rows = rows;

    int buf_size = cols * rows * 2;
    textbuf = (uint8_t *)malloc(buf_size);
    shadow_buf = (uint8_t *)malloc(buf_size);
    if (!textbuf || !shadow_buf) return;

    /* Fill with spaces, default attribute (light gray on black) */
    uint8_t def_attr = (COLOR_BLACK << 4) | COLOR_LIGHT_GRAY;
    for (int i = 0; i < cols * rows; i++) {
        textbuf[i * 2]     = ' ';
        textbuf[i * 2 + 1] = def_attr;
    }
    /* Mark shadow as all-different to force initial full paint */
    memset(shadow_buf, 0xFF, buf_size);

    cursor_col = 0;
    cursor_row = 0;
    cur_fg = COLOR_LIGHT_GRAY;
    cur_bg = COLOR_BLACK;
    bold_on = false;
    reverse_on = false;
    cursor_visible = true;
    vt_state = ST_NORMAL;
    last_fb_ptr = NULL;

    /* Input ring buffer */
    in_head = 0;
    in_tail = 0;
    in_sem = xSemaphoreCreateCounting(VT100_INBUF_SIZE, 0);
}

void vt100_destroy(void) {
    if (textbuf) { free(textbuf); textbuf = NULL; }
    if (shadow_buf) { free(shadow_buf); shadow_buf = NULL; }
    if (in_sem) { vSemaphoreDelete(in_sem); in_sem = NULL; }
}

void vt100_set_hwnd(hwnd_t hwnd) {
    g_vt_hwnd = hwnd;
}

/* ═════════════════════════════════════════════════════════════════════════
 * Resize
 * ═════════════════════════════════════════════════════════════════════════ */

void vt100_resize(int cols, int rows) {
    if (cols > VT100_MAX_COLS) cols = VT100_MAX_COLS;
    if (rows > VT100_MAX_ROWS) rows = VT100_MAX_ROWS;
    if (cols == tb_cols && rows == tb_rows) return;

    int new_size = cols * rows * 2;
    uint8_t *new_buf = (uint8_t *)malloc(new_size);
    uint8_t *new_shadow = (uint8_t *)malloc(new_size);
    if (!new_buf || !new_shadow) {
        if (new_buf) free(new_buf);
        if (new_shadow) free(new_shadow);
        return;
    }

    uint8_t def_attr = (COLOR_BLACK << 4) | COLOR_LIGHT_GRAY;
    for (int i = 0; i < cols * rows; i++) {
        new_buf[i * 2]     = ' ';
        new_buf[i * 2 + 1] = def_attr;
    }

    /* Copy what fits from old buffer */
    int copy_rows = (rows < tb_rows) ? rows : tb_rows;
    int copy_cols = (cols < tb_cols) ? cols : tb_cols;
    for (int r = 0; r < copy_rows; r++)
        for (int c = 0; c < copy_cols; c++) {
            int old_off = (r * tb_cols + c) * 2;
            int new_off = (r * cols + c) * 2;
            new_buf[new_off]     = textbuf[old_off];
            new_buf[new_off + 1] = textbuf[old_off + 1];
        }

    memset(new_shadow, 0xFF, new_size); /* force full repaint */

    free(textbuf);
    free(shadow_buf);
    textbuf = new_buf;
    shadow_buf = new_shadow;
    tb_cols = cols;
    tb_rows = rows;

    if (cursor_col >= cols) cursor_col = cols - 1;
    if (cursor_row >= rows) cursor_row = rows - 1;
    last_fb_ptr = NULL;
}

void vt100_get_size(int *cols, int *rows) {
    *cols = tb_cols;
    *rows = tb_rows;
}

/* ═════════════════════════════════════════════════════════════════════════
 * Cursor blink
 * ═════════════════════════════════════════════════════════════════════════ */

void vt100_toggle_cursor(void) {
    cursor_visible = !cursor_visible;
    invalidate();
}

/* ═════════════════════════════════════════════════════════════════════════
 * Paint callback — render terminal buffer to window framebuffer
 *
 * Framebuffer format: 4-bit nibble-packed
 *   high nibble = left (even-x) pixel, low nibble = right (odd-x) pixel
 *   stride = FB_STRIDE (320) bytes per row
 *
 * font_8x16[] glyphs: LSB = leftmost pixel (matches MMBasic pattern)
 * ═════════════════════════════════════════════════════════════════════════ */

void vt100_paint(hwnd_t hwnd) {
    if (!textbuf) return;

    wd_begin(hwnd);
    int16_t stride;
    uint8_t *dst = wd_fb_ptr(0, 0, &stride);
    if (!dst) {
        wd_end();
        return;
    }

    /* Compute visible area for clipping */
    int16_t clip_w, clip_h;
    wd_get_clip_size(&clip_w, &clip_h);
    int vis_cols = tb_cols;
    int vis_rows = tb_rows;
    if (vis_cols * VT100_FONT_W > clip_w)
        vis_cols = clip_w / VT100_FONT_W;
    if (vis_rows * VT100_FONT_H > clip_h)
        vis_rows = clip_h / VT100_FONT_H;

    if (vis_cols == 0 || vis_rows == 0) {
        wd_end();
        return;
    }

    /* Detect window move → force full repaint */
    bool force = (dst != last_fb_ptr);
    last_fb_ptr = dst;

    for (int row = 0; row < vis_rows; row++) {
        for (int col = 0; col < vis_cols; col++) {
            int off = TB_OFF(row, col);
            uint8_t ch   = textbuf[off];
            uint8_t attr = textbuf[off + 1];

            /* Cursor: invert fg/bg on current cell when visible */
            bool cursor = (row == cursor_row && col == cursor_col && cursor_visible);
            uint8_t eff_attr = cursor ? (uint8_t)((attr >> 4) | (attr << 4))
                                      : attr;

            /* Skip unchanged cells */
            int soff = (row * vis_cols + col) * 2;
            if (soff + 1 < tb_cols * tb_rows * 2) {
                if (!force &&
                    shadow_buf[off] == ch &&
                    shadow_buf[off + 1] == eff_attr)
                    continue;
                shadow_buf[off]     = ch;
                shadow_buf[off + 1] = eff_attr;
            }

            uint8_t fg = eff_attr & 0x0Fu;
            uint8_t bg = (eff_attr >> 4) & 0x0Fu;
            const uint8_t *glyph = &font_8x16[(uint8_t)ch * VT100_FONT_H];

            /* Render 8×16 glyph.
             * Column pixel offset is col*8, always even → fast path. */
            for (int gy = 0; gy < VT100_FONT_H; gy++) {
                uint8_t bits = glyph[gy];
                uint8_t *drow = dst
                              + (row * VT100_FONT_H + gy) * stride
                              + (col * VT100_FONT_W) / 2;

                /* LSB = leftmost pixel: process 2 bits at a time → 4 bytes per row */
                for (int bx = 0; bx < VT100_FONT_W / 2; bx++) {
                    uint8_t left  = (bits & 0x01) ? fg : bg;
                    uint8_t right = (bits & 0x02) ? fg : bg;
                    *drow++ = (left << 4) | right;
                    bits >>= 2;
                }
            }
        }
    }

    wd_end();
}
