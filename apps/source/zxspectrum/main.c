/*
 * FRANK OS — ZX Spectrum 48K Emulator (standalone ELF app)
 *
 * Uses zx2040 by antirez (https://github.com/antirez/zx2040) as the
 * emulation core: header-only Z80 CPU + ZX Spectrum ULA emulation.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "m-os-api.h"
#include "frankos-app.h"

/*
 * m-os-api.h defines:
 *   #define switch DO_NOT_USE_SWITCH
 *   #define inline __force_inline
 * These break the zx2040 headers which use switch statements and inline
 * functions extensively.  Undo them before pulling in the emulator core.
 */
#undef switch
#undef inline
#undef __force_inline

/* Disable audio — SPEAKER_PIN == -1 skips audio sampling in zx_exec */
#define SPEAKER_PIN -1

/* Override assert for freestanding environment */
#define CHIPS_ASSERT(c) ((void)(c))

/* mem.h calls vram_set_dirty_*() for SPI display tracking (zx2040).
 * We don't use partial-scanline tracking, so provide no-op stubs. */
static inline void vram_set_dirty_bitmap(uint16_t addr) { (void)addr; }
static inline void vram_set_dirty_attr(uint16_t addr)   { (void)addr; }

/* Pull in the emulator implementation.
 * Include order matches zx.c: chips_common → mem → z80 → kbd → clk → zx
 *
 * Compile entire emulation core at -O2 for speed (the hot path is z80_tick
 * called from _zx_tick called from zx_exec — all must be fast). */
#define CHIPS_IMPL
#pragma GCC optimize("O2")
#include "chips_common.h"
#include "mem.h"
#include "z80.h"
#include "kbd.h"
#include "clk.h"
#include "zx.h"
#pragma GCC optimize("Oz")
#include "zx-roms.h"

/* ======================================================================
 * Emulator state
 * ====================================================================== */

static zx_t         *zx;          /* heap-allocated in SRAM */
static hwnd_t        app_hwnd;
static volatile bool closing;
static void         *app_task;

/* Border: 32 px left/right, 24 px top/bottom → 320×240 client area */
#define BORDER_H  32
#define BORDER_V  24
#define CLIENT_W  (256 + 2 * BORDER_H)   /* 320 */
#define CLIENT_H  (192 + 2 * BORDER_V)   /* 240 */

/* ======================================================================
 * Colour mapping:  ZX 16-colour → CGA 16-colour
 * ====================================================================== */

static const uint8_t zx_to_cga[16] = {
    COLOR_BLACK,          /*  0  Black         */
    COLOR_BLUE,           /*  1  Blue          */
    COLOR_RED,            /*  2  Red           */
    COLOR_MAGENTA,        /*  3  Magenta       */
    COLOR_GREEN,          /*  4  Green         */
    COLOR_CYAN,           /*  5  Cyan          */
    COLOR_BROWN,          /*  6  Yellow (dim)  */
    COLOR_LIGHT_GRAY,     /*  7  White  (dim)  */
    COLOR_BLACK,          /*  8  Bright Black  */
    COLOR_LIGHT_BLUE,     /*  9  Bright Blue   */
    COLOR_LIGHT_RED,      /* 10  Bright Red    */
    COLOR_LIGHT_MAGENTA,  /* 11  Bright Magenta*/
    COLOR_LIGHT_GREEN,    /* 12  Bright Green  */
    COLOR_LIGHT_CYAN,     /* 13  Bright Cyan   */
    COLOR_YELLOW,         /* 14  Bright Yellow */
    COLOR_WHITE,          /* 15  Bright White  */
};

/* ======================================================================
 * Paint handler — 1× native resolution (256×192) + border
 * ====================================================================== */

static void zx_paint(hwnd_t hwnd) {
    wd_begin(hwnd);

    uint8_t border = zx_to_cga[zx->border_color & 7];

    /* Draw border — four rectangles around the 256×192 bitmap area */
    wd_fill_rect(0, 0, CLIENT_W, BORDER_V, border);                       /* top */
    wd_fill_rect(0, BORDER_V + 192, CLIENT_W, BORDER_V, border);          /* bottom */
    wd_fill_rect(0, BORDER_V, BORDER_H, 192, border);                     /* left */
    wd_fill_rect(BORDER_H + 256, BORDER_V, BORDER_H, 192, border);        /* right */

    /* Decode ZX VRAM at native 1× resolution */
    uint8_t *vmem = zx->ram[0];

    for (int y = 0; y < 192; y++) {
        /* ZX Spectrum VRAM address interleaving */
        int addr = ((y & 0xC0) << 5) | ((y & 0x07) << 8) | ((y & 0x38) << 2);
        int py = BORDER_V + y;

        /* Walk 32 columns × 8 pixels, batching runs of same colour */
        uint8_t run_color = 0xFF;   /* sentinel */
        int     run_start = BORDER_H;

        for (int col = 0; col < 32; col++) {
            uint8_t byte = vmem[addr + col];
            uint8_t attr = vmem[0x1800 + ((y >> 3) << 5) + col];

            uint8_t ink    = attr & 0x07;
            uint8_t paper  = (attr >> 3) & 0x07;
            uint8_t bright = (attr & 0x40) ? 8 : 0;
            if ((attr & 0x80) && (zx->blink_counter & 0x10)) {
                uint8_t tmp = ink; ink = paper; paper = tmp;
            }
            uint8_t fg = zx_to_cga[ink + bright];
            uint8_t bg = zx_to_cga[paper + bright];

            for (int bit = 7; bit >= 0; bit--) {
                uint8_t color = (byte & (1 << bit)) ? fg : bg;
                int px = BORDER_H + col * 8 + (7 - bit);

                if (color != run_color) {
                    if (run_color != 0xFF) {
                        wd_hline(run_start, py, px - run_start, run_color);
                    }
                    run_color = color;
                    run_start = px;
                }
            }
        }
        /* Flush final run */
        if (run_color != 0xFF) {
            wd_hline(run_start, py, BORDER_H + 256 - run_start, run_color);
        }
    }
    wd_end();
}

/* ======================================================================
 * Keyboard mapping:  HID scancode → ZX key code
 * ====================================================================== */

static int hid_to_zx(uint8_t scancode) {
    if (scancode >= 0x04 && scancode <= 0x1D)     /* a-z */
        return 'a' + (scancode - 0x04);
    if (scancode >= 0x1E && scancode <= 0x26)      /* 1-9 */
        return '1' + (scancode - 0x1E);
    if (scancode == 0x27) return '0';
    if (scancode == 0x28) return 0x0D;             /* Enter */
    if (scancode == 0x2C) return ' ';              /* Space */
    if (scancode == 0x2A) return 0x0C;             /* Backspace → Delete */
    if (scancode == 0x50) return 0x08;             /* Left */
    if (scancode == 0x4F) return 0x09;             /* Right */
    if (scancode == 0x51) return 0x0A;             /* Down */
    if (scancode == 0x52) return 0x0B;             /* Up */
    return -1;
}

static uint8_t prev_modifiers;

static void handle_modifiers(uint8_t mods, bool is_down) {
    (void)is_down;
    uint8_t changed = mods ^ prev_modifiers;
    prev_modifiers = mods;

    if (changed & KMOD_SHIFT) {
        if (mods & KMOD_SHIFT) zx_key_down(zx, 0x00);
        else                   zx_key_up(zx, 0x00);
    }
    if (changed & KMOD_CTRL) {
        if (mods & KMOD_CTRL)  zx_key_down(zx, 0x0F);
        else                   zx_key_up(zx, 0x0F);
    }
}

/* ======================================================================
 * Event handler
 * ====================================================================== */

static bool zx_event(hwnd_t hwnd, const window_event_t *ev) {
    if (ev->type == WM_KEYDOWN || ev->type == WM_KEYUP) {
        int zx_key = hid_to_zx(ev->key.scancode);
        if (zx_key >= 0) {
            if (ev->type == WM_KEYDOWN) zx_key_down(zx, zx_key);
            else                         zx_key_up(zx, zx_key);
        }
        handle_modifiers(ev->key.modifiers, ev->type == WM_KEYDOWN);
        return true;
    }

    if (ev->type == WM_CLOSE) {
        closing = true;
        xTaskNotifyGive(app_task);
        return true;
    }

    return false;
}

/* ======================================================================
 * Entry point
 * ====================================================================== */

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    app_task = xTaskGetCurrentTaskHandle();

    /* Allocate zx_t in SRAM */
    zx = (zx_t *)pvPortMalloc(sizeof(zx_t));
    if (!zx) {
        goutf("ZX Spectrum: not enough SRAM for emulator (%u bytes)\n",
              (unsigned)sizeof(zx_t));
        return 1;
    }
    memset(zx, 0, sizeof(zx_t));

    /* Initialise ZX Spectrum 48K emulator */
    zx_desc_t desc;
    memset(&desc, 0, sizeof(desc));
    desc.type = ZX_TYPE_48K;
    desc.joystick_type = ZX_JOYSTICKTYPE_NONE;
    desc.roms.zx48k.ptr = dump_amstrad_zx48k_bin;
    desc.roms.zx48k.size = sizeof(dump_amstrad_zx48k_bin);
    zx_init(zx, &desc);

    /* Window: 320×240 client area (256×192 + border) */
    int16_t fw = CLIENT_W + 2 * THEME_BORDER_WIDTH;
    int16_t fh = CLIENT_H + THEME_TITLE_HEIGHT + 2 * THEME_BORDER_WIDTH;
    int16_t x  = (DISPLAY_WIDTH  - fw) / 2;
    int16_t y  = (DISPLAY_HEIGHT - TASKBAR_HEIGHT - fh) / 2;
    if (y < 0) y = 0;

    app_hwnd = wm_create_window(x, y, fw, fh, "ZX Spectrum",
                                WSTYLE_DIALOG, zx_event, zx_paint);
    if (app_hwnd == HWND_NULL) {
        vPortFree(zx);
        return 1;
    }

    wm_show_window(app_hwnd);
    wm_set_focus(app_hwnd);
    taskbar_invalidate();

    serial_printf("ZX: SP=%p zx=%p sizeof(zx_t)=%u\n",
                  __builtin_frame_address(0), (void *)zx,
                  (unsigned)sizeof(zx_t));

    /* Main emulation loop.
     * zx_exec() takes MICROSECONDS — 20000 µs = 20 ms = 1 ZX frame. */
    uint32_t frame = 0;
    uint32_t t0 = xTaskGetTickCount();

    while (!closing) {
        /* Run z80 in short bursts with interrupts disabled to cut ISR
         * overhead.  8 × 2500 µs = 20000 µs = one 50-Hz frame.
         * Max interrupt latency ≈ 4 ms — safe for USB keyboard polling. */
        for (int burst = 0; burst < 8 && !closing; burst++) {
            __asm volatile("cpsid i");
            zx_exec(zx, 2500);
            __asm volatile("cpsie i");
        }

        if ((++frame & 3) == 0)
            wm_invalidate(app_hwnd);

        vTaskDelay(pdMS_TO_TICKS(1));

        if (frame == 50) {
            uint32_t elapsed = xTaskGetTickCount() - t0;
            serial_printf("ZX: 50fr %ums (%u fps)\n",
                          (unsigned)elapsed,
                          (unsigned)(50000 / elapsed));
        }
    }

    wm_destroy_window(app_hwnd);
    taskbar_invalidate();
    vPortFree(zx);
    return 0;
}
