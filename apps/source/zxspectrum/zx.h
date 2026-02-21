#pragma once
/*#
    # zx.h

    A ZX Spectrum 48K / 128 emulator in a C header.

    Do this:
    ~~~C
    #define CHIPS_IMPL
    ~~~
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following macros with your own implementation

    ~~~C
    CHIPS_ASSERT(c)
    ~~~
        your own assert macro (default: assert(c))

    You need to include the following headers before including zx.h:

    - chips/chips_common.h
    - chips/z80.h
    - chips/ay38910.h
    - chips/mem.h
    - chips/kbd.h
    - chips/clk.h

    ## The ZX Spectrum 48K

    TODO!

    ## The ZX Spectrum 128

    TODO!

    ## TODO:
    - 'contended memory' timing and IO port timing
    - reads from port 0xFF must return 'current VRAM bytes
    - video decoding only has scanline accuracy, not pixel accuracy

    ## zlib/libpng license

    Copyright (c) 2018 Andre Weissflog
    Copyright (c) 2024 Salvatore Sanfilippo
    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.
    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:
        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.
        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.
        3. This notice may not be removed or altered from any source
        distribution.
#*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

// bump this whenever the zx_t struct layout changes
#define ZX_SNAPSHOT_VERSION (0x0001)

#define ZX_FRAMEBUFFER_WIDTH (320/2) // 4 bits per pixel.
#define ZX_FRAMEBUFFER_HEIGHT (256)
#define ZX_FRAMEBUFFER_SIZE_BYTES (ZX_FRAMEBUFFER_WIDTH * ZX_FRAMEBUFFER_HEIGHT)
#define ZX_DISPLAY_WIDTH (320)
#define ZX_DISPLAY_HEIGHT (256)

// ZX Spectrum models
typedef enum {
    ZX_TYPE_48K,
} zx_type_t;

// ZX Spectrum joystick types
typedef enum {
    ZX_JOYSTICKTYPE_NONE,
    ZX_JOYSTICKTYPE_KEMPSTON,
    ZX_JOYSTICKTYPE_SINCLAIR_1,
    ZX_JOYSTICKTYPE_SINCLAIR_2,
} zx_joystick_type_t;

// joystick mask bits
#define ZX_JOYSTICK_RIGHT   (1<<0)
#define ZX_JOYSTICK_LEFT    (1<<1)
#define ZX_JOYSTICK_DOWN    (1<<2)
#define ZX_JOYSTICK_UP      (1<<3)
#define ZX_JOYSTICK_BTN     (1<<4)

// config parameters for zx_init()
typedef struct {
    zx_type_t type;                     // default is ZX_TYPE_48K
    zx_joystick_type_t joystick_type;   // what joystick to emulate, default is ZX_JOYSTICK_NONE
    // ROM images
    struct {
        // ZX Spectrum 48K
        chips_range_t zx48k;
    } roms;
} zx_desc_t;

// ZX emulator state
typedef struct {
    z80_t cpu;
    zx_type_t type;
    zx_joystick_type_t joystick_type;
    bool memory_paging_disabled;
    uint8_t kbd_joymask;        // joystick mask from keyboard joystick emulation
    uint8_t joy_joymask;        // joystick mask from zx_joystick()
    uint32_t tick_count;
    uint8_t last_mem_config;    // last out to 0x7FFD
    uint8_t last_fe_out;        // last out value to 0xFE port
    uint8_t blink_counter;      // incremented on each vblank
    uint8_t border_color;
    int frame_scan_lines;
    int top_border_scanlines;
    int scanline_period;
    int scanline_counter;
    int scanline_y;

    // Audio state: this is RP2040 specific code. We sample the speaker
    // value in a bitmap (audiobuf), since anyway the Spectrum sound is
    // 1 bit, even if at high resolution.
#define AUDIOBUF_LEN 256 // Must be power of 2
    int beeper_state;           // Last value written to the speaker bit.
    uint32_t audiobuf[AUDIOBUF_LEN];    // 1 bit samples audio buffer.
    uint32_t audiobuf_byte;             // Current byte to write.
    uint32_t audiobuf_bit;              // Current bit to write.
    volatile uint32_t audiobuf_notify;  // Just a brutal inter-process signal.

    int int_counter;
    uint32_t display_ram_bank;
    kbd_t kbd;
    mem_t mem;
    uint64_t pins;
    uint64_t freq_hz;
    bool valid;
    uint8_t ram[3][0x4000];
    uint8_t rom[1][0x4000];
} zx_t;

// initialize a new ZX Spectrum instance
void zx_init(zx_t* sys, const zx_desc_t* desc);
// discard a ZX Spectrum instance
void zx_discard(zx_t* sys);
// reset a ZX Spectrum instance
void zx_reset(zx_t* sys);
// query information about display requirements, can be called with nullptr
chips_display_info_t zx_display_info(zx_t* sys);
// run ZX Spectrum instance for a given number of microseconds, return number of ticks
uint32_t zx_exec(zx_t* sys, uint32_t micro_seconds);
// send a key-down event
void zx_key_down(zx_t* sys, int key_code);
// send a key-up event
void zx_key_up(zx_t* sys, int key_code);
// enable/disable joystick emulation
void zx_set_joystick_type(zx_t* sys, zx_joystick_type_t type);
// get current joystick emulation type
zx_joystick_type_t zx_joystick_type(zx_t* sys);
// set joystick mask (combination of ZX_JOYSTICK_*)
void zx_joystick(zx_t* sys, uint8_t mask);
// load a ZX Z80 file into the emulator
bool zx_quickload(zx_t* sys, chips_range_t data);
// save a snapshot, patches any pointers to zero, returns a snapshot version
uint32_t zx_save_snapshot(zx_t* sys, zx_t* dst);
// load a snapshot, returns false if snapshot version doesn't match
bool zx_load_snapshot(zx_t* sys, uint32_t version, zx_t* src);

#ifdef __cplusplus
} // extern "C"
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef CHIPS_IMPL
#include <string.h>
#ifndef CHIPS_ASSERT
    #include <assert.h>
    #define CHIPS_ASSERT(c) assert(c)
#endif

static void _zx_init_memory_map(zx_t* sys);
static void _zx_init_keyboard_matrix(zx_t* sys);

#define _ZX_DEFAULT(val,def) (((val) != 0) ? (val) : (def))

#define _ZX_48K_FREQUENCY (3500000)

void zx_init(zx_t* sys, const zx_desc_t* desc) {
    CHIPS_ASSERT(sys && desc);

    memset(sys, 0, sizeof(zx_t));
    sys->valid = true;
    sys->type = desc->type;
    sys->joystick_type = desc->joystick_type;
    sys->freq_hz = _ZX_48K_FREQUENCY;

    // initalize the hardware
    sys->border_color = 0;
    CHIPS_ASSERT(desc->roms.zx48k.ptr && (desc->roms.zx48k.size == 0x4000));
    memcpy(sys->rom[0], desc->roms.zx48k.ptr, 0x4000);
    sys->display_ram_bank = 0;
    sys->frame_scan_lines = 312;
    sys->top_border_scanlines = 64;
    sys->scanline_period = 224; // This value is modified in zx.c
    sys->scanline_counter = sys->scanline_period;

    sys->pins = z80_init(&sys->cpu);

    _zx_init_memory_map(sys);
    _zx_init_keyboard_matrix(sys);

    // Audio initialization
    memset(sys->audiobuf,0,sizeof(sys->audiobuf));
    sys->audiobuf_byte = 0;
    sys->audiobuf_bit = 0;
    sys->audiobuf_notify = 0;
}

void zx_discard(zx_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->valid = false;
}

void zx_reset(zx_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->pins = z80_reset(&sys->cpu);
    sys->memory_paging_disabled = false;
    sys->kbd_joymask = 0;
    sys->joy_joymask = 0;
    sys->last_fe_out = 0;
    sys->scanline_counter = sys->scanline_period;
    sys->scanline_y = 0;
    sys->blink_counter = 0;
    sys->display_ram_bank = 0;
    _zx_init_memory_map(sys);
}

static uint64_t _zx_tick(zx_t* sys, uint64_t pins) {
    pins = z80_tick(&sys->cpu, &sys->mem, pins);

    // video decoding and vblank interrupt
    if (--sys->scanline_counter <= 0) {
        // We don't do any actual video decoding: the emulator
        // reads directly from the Spectrum video RAM. Yet we
        // have to track which scanline our non-existing CRT
        // is decoding, and request the vertical blank interrupt
        // when we are at the end.
        sys->scanline_counter += sys->scanline_period;
        if (sys->scanline_y++ >= sys->frame_scan_lines) {
            // start new frame, request vblank interrupt
            sys->scanline_y = 0;
            sys->blink_counter++;
            // request vblank interrupt
            pins |= Z80_INT;
            // hold the INT pin for 32 ticks
            sys->int_counter = 32;
        }
    }

    // clear INT pin after 32 ticks
    if (pins & Z80_INT) {
        if (--sys->int_counter < 0) {
            pins &= ~Z80_INT;
        }
    }

    if (pins & Z80_MREQ) {
        // a memory request
        // FIXME: 'contended memory'
        const uint16_t addr = Z80_GET_ADDR(pins);
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem_rd(&sys->mem, addr));
        }
        else if (pins & Z80_WR) {
            mem_wr(&sys->mem, addr, Z80_GET_DATA(pins));
        }
    }
    else if (pins & Z80_IORQ) {
        if ((pins & Z80_A0) == 0) {
            /* Spectrum ULA (...............0)
                Bits 5 and 7 as read by INning from Port 0xfe are always one
            */
            if (pins & Z80_RD) {
                // read from ULA
                uint8_t data = (1<<7)|(1<<5);
                // MIC/EAR flags -> bit 6
                if (sys->last_fe_out & (1<<3|1<<4)) {
                    data |= (1<<6);
                }
                // keyboard matrix bits are encoded in the upper 8 bit of the port address
                uint16_t column_mask = (~(Z80_GET_ADDR(pins)>>8)) & 0x00FF;
                const uint16_t kbd_lines = kbd_test_lines(&sys->kbd, column_mask);
                data |= (~kbd_lines) & 0x1F;
                Z80_SET_DATA(pins, data);
            }
            else if (pins & Z80_WR) {
                // write to ULA
                // FIXME: bit 3: MIC output (CAS SAVE, 0=On, 1=Off)
                const uint8_t data = Z80_GET_DATA(pins);
                sys->border_color = data & 7;
                sys->last_fe_out = data;

                // Replicate the Z80 audio pin status on the global state
                // so we can sample it at regular intervals.
                sys->beeper_state = 0 != (data & (1<<4));
            }
        }
        else if ((pins & (Z80_RD|Z80_A7|Z80_A6|Z80_A5)) == Z80_RD) {
            // Kempston Joystick (........000.....)
            Z80_SET_DATA(pins, sys->kbd_joymask | sys->joy_joymask);
        }
    }

    return pins;
}

uint32_t zx_exec(zx_t* sys, uint32_t micro_seconds) {
    CHIPS_ASSERT(sys && sys->valid);
    const uint32_t num_ticks = clk_us_to_ticks(sys->freq_hz, micro_seconds);
    uint64_t pins = sys->pins;
    const uint32_t last_bitmap_scanline = 64+192;

    // Other than the ticks, we have another stop condition, that is to stop
    // only when a final bitmap scanline was reached. This is useful because the
    // emulator updates the ST77xx display directly from the Spectrum VRAM.
    //
    // Some games try to update the screen during the vblank interrupt, and
    // sometimes kinda try to "follow" the raster with precise timing.
    // To update the screen at the end of the bitmap part, before the border,
    // looks like a decent compromise in pratice, testing with a few games
    // requiring precise timing.
    //
    // For games with specific needs, we will override the scanline period
    // to give them a bit of extra time, or less time if they start deleting
    // the old sprites too early.
    for (uint32_t tick = 0; tick < num_ticks || sys->scanline_y != last_bitmap_scanline; tick++) {
        pins = _zx_tick(sys, pins);

        // Audio buffer handling.
        if (SPEAKER_PIN != -1 && !(tick & 0xf)) {
            // Fill sample.
            sys->audiobuf[sys->audiobuf_byte] &=
                ~(((uint32_t)1)<<sys->audiobuf_bit);
            sys->audiobuf[sys->audiobuf_byte] |=
                sys->beeper_state<<sys->audiobuf_bit;

            // Go to next byte/bit
            sys->audiobuf_bit = (sys->audiobuf_bit+1) & 31; // Incr modulo 32.
            if (sys->audiobuf_bit == 0)
                sys->audiobuf_byte = (sys->audiobuf_byte+1) & (AUDIOBUF_LEN-1);

            // Buffer full (back to zero after increment)? Set the timestamp
            // and ping the other thread that plays the samples.
            if ((sys->audiobuf_byte == 0 ||
                 sys->audiobuf_byte == AUDIOBUF_LEN/2) &&
                 sys->audiobuf_bit == 0)
            {
                // audiobuf_notify will be cleared by other thread.
                sys->audiobuf_notify = sys->audiobuf_byte == 0 ? 2 : 1;
            }
        }
    }
    sys->pins = pins;
    kbd_update(&sys->kbd, micro_seconds);
    return num_ticks;
}

void zx_key_down(zx_t* sys, int key_code) {
    CHIPS_ASSERT(sys && sys->valid);
    switch (sys->joystick_type) {
        case ZX_JOYSTICKTYPE_NONE:
            kbd_key_down(&sys->kbd, key_code);
            break;
        case ZX_JOYSTICKTYPE_KEMPSTON:
            switch (key_code) {
                case 0xFF:  sys->kbd_joymask |= ZX_JOYSTICK_BTN; break;
                case 0xFE:  sys->kbd_joymask |= ZX_JOYSTICK_LEFT; break;
                case 0xFD:  sys->kbd_joymask |= ZX_JOYSTICK_RIGHT; break;
                case 0xFC:  sys->kbd_joymask |= ZX_JOYSTICK_DOWN; break;
                case 0xFB:  sys->kbd_joymask |= ZX_JOYSTICK_UP; break;
                default:    kbd_key_down(&sys->kbd, key_code); break;
            }
            break;
        // the Sinclair joystick ports work as normal keys
        case ZX_JOYSTICKTYPE_SINCLAIR_1:
            switch (key_code) {
                case 0xFF:  key_code = '5'; break;    // fire
                case 0xFE:  key_code = '1'; break;    // left
                case 0xFD:  key_code = '2'; break;    // right
                case 0xFC:  key_code = '3'; break;    // down
                case 0xFB:  key_code = '4'; break;    // up
                default: break;
            }
            kbd_key_down(&sys->kbd, key_code);
            break;
        case ZX_JOYSTICKTYPE_SINCLAIR_2:
            switch (key_code) {
                case 0xFF:  key_code = '0'; break;    // fire
                case 0xFE:  key_code = '6'; break;    // left
                case 0xFD:  key_code = '7'; break;    // right
                case 0xFC:  key_code = '8'; break;    // down
                case 0xFB:  key_code = '9'; break;    // up
                default: break;
            }
            kbd_key_down(&sys->kbd, key_code);
            break;
    }
}

void zx_key_up(zx_t* sys, int key_code) {
    CHIPS_ASSERT(sys && sys->valid);
    switch (sys->joystick_type) {
        case ZX_JOYSTICKTYPE_NONE:
            kbd_key_up(&sys->kbd, key_code);
            break;
        case ZX_JOYSTICKTYPE_KEMPSTON:
            switch (key_code) {
                case 0xFF:  sys->kbd_joymask &= ~ZX_JOYSTICK_BTN; break;
                case 0xFE:  sys->kbd_joymask &= ~ZX_JOYSTICK_LEFT; break;
                case 0xFD:  sys->kbd_joymask &= ~ZX_JOYSTICK_RIGHT; break;
                case 0xFC:  sys->kbd_joymask &= ~ZX_JOYSTICK_DOWN; break;
                case 0xFB:  sys->kbd_joymask &= ~ZX_JOYSTICK_UP; break;
                default:    kbd_key_up(&sys->kbd, key_code); break;
            }
            break;
        // the Sinclair joystick ports work as normal keys
        case ZX_JOYSTICKTYPE_SINCLAIR_1:
            switch (key_code) {
                case 0xFF:  key_code = '5'; break;    // fire
                case 0xFE:  key_code = '1'; break;    // left
                case 0xFD:  key_code = '2'; break;    // right
                case 0xFC:  key_code = '3'; break;    // down
                case 0xFB:  key_code = '4'; break;    // up
                default: break;
            }
            kbd_key_up(&sys->kbd, key_code);
            break;
        case ZX_JOYSTICKTYPE_SINCLAIR_2:
            switch (key_code) {
                case 0xFF:  key_code = '0'; break;    // fire
                case 0xFE:  key_code = '6'; break;    // left
                case 0xFD:  key_code = '7'; break;    // right
                case 0xFC:  key_code = '8'; break;    // down
                case 0xFB:  key_code = '9'; break;    // up
                default: break;
            }
            kbd_key_up(&sys->kbd, key_code);
            break;
    }
}

void zx_set_joystick_type(zx_t* sys, zx_joystick_type_t type) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->joystick_type = type;
}

zx_joystick_type_t zx_joystick_type(zx_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    return sys->joystick_type;
}

void zx_joystick(zx_t* sys, uint8_t mask) {
    CHIPS_ASSERT(sys && sys->valid);
    if (sys->joystick_type == ZX_JOYSTICKTYPE_SINCLAIR_1) {
        if (mask & ZX_JOYSTICK_BTN)   { kbd_key_down(&sys->kbd, '5'); }
        else                          { kbd_key_up(&sys->kbd, '5'); }
        if (mask & ZX_JOYSTICK_LEFT)  { kbd_key_down(&sys->kbd, '1'); }
        else                          { kbd_key_up(&sys->kbd, '1'); }
        if (mask & ZX_JOYSTICK_RIGHT) { kbd_key_down(&sys->kbd, '2'); }
        else                          { kbd_key_up(&sys->kbd, '2'); }
        if (mask & ZX_JOYSTICK_DOWN)  { kbd_key_down(&sys->kbd, '3'); }
        else                          { kbd_key_up(&sys->kbd, '3'); }
        if (mask & ZX_JOYSTICK_UP)    { kbd_key_down(&sys->kbd, '4'); }
        else                          { kbd_key_up(&sys->kbd, '4'); }
    }
    else if (sys->joystick_type == ZX_JOYSTICKTYPE_SINCLAIR_2) {
        if (mask & ZX_JOYSTICK_BTN)   { kbd_key_down(&sys->kbd, '0'); }
        else                          { kbd_key_up(&sys->kbd, '0'); }
        if (mask & ZX_JOYSTICK_LEFT)  { kbd_key_down(&sys->kbd, '6'); }
        else                          { kbd_key_up(&sys->kbd, '6'); }
        if (mask & ZX_JOYSTICK_RIGHT) { kbd_key_down(&sys->kbd, '7'); }
        else                          { kbd_key_up(&sys->kbd, '7'); }
        if (mask & ZX_JOYSTICK_DOWN)  { kbd_key_down(&sys->kbd, '8'); }
        else                          { kbd_key_up(&sys->kbd, '8'); }
        if (mask & ZX_JOYSTICK_UP)    { kbd_key_down(&sys->kbd, '9'); }
        else                          { kbd_key_up(&sys->kbd, '9'); }
    }
    else {
        sys->joy_joymask = mask;
    }
}

static void _zx_init_memory_map(zx_t* sys) {
    mem_init(&sys->mem);
    mem_map_ram(&sys->mem, 0, 0x4000, 0x4000, sys->ram[0]);
    mem_map_ram(&sys->mem, 0, 0x8000, 0x4000, sys->ram[1]);
    mem_map_ram(&sys->mem, 0, 0xC000, 0x4000, sys->ram[2]);
    mem_map_rom(&sys->mem, 0, 0x0000, 0x4000, sys->rom[0]);
}

static void _zx_init_keyboard_matrix(zx_t* sys) {
    // setup keyboard matrix
    kbd_init(&sys->kbd, 1);
    // caps-shift is column 0, line 0
    kbd_register_modifier(&sys->kbd, 0, 0, 0);
    // sym-shift is column 7, line 1
    kbd_register_modifier(&sys->kbd, 1, 7, 1);
    // alpha-numeric keys
    const char* keymap =
        /* no shift */
        " zxcv"         // A8       shift,z,x,c,v
        "asdfg"         // A9       a,s,d,f,g
        "qwert"         // A10      q,w,e,r,t
        "12345"         // A11      1,2,3,4,5
        "09876"         // A12      0,9,8,7,6
        "poiuy"         // A13      p,o,i,u,y
        " lkjh"         // A14      enter,l,k,j,h
        "  mnb"         // A15      space,symshift,m,n,b

        // shift
        " ZXCV"         // A8
        "ASDFG"         // A9
        "QWERT"         // A10
        "     "         // A11
        "     "         // A12
        "POIUY"         // A13
        " LKJH"         // A14
        "  MNB"         // A15

        // symshift
        " : ?/"         // A8
        "     "         // A9
        "   <>"         // A10
        "!@#$%"         // A11
        "_)('&"         // A12
        "\";   "        // A13
        " =+-^"         // A14
        "  .,*";        // A15
    for (int layer = 0; layer < 3; layer++) {
        for (int column = 0; column < 8; column++) {
            for (int line = 0; line < 5; line++) {
                const uint8_t c = keymap[layer*40 + column*5 + line];
                if (c != 0x20) {
                    kbd_register_key(&sys->kbd, c, column, line, (layer>0) ? (1<<(layer-1)) : 0);
                }
            }
        }
    }

    // special keys
    kbd_register_key(&sys->kbd, ' ', 7, 0, 0);  // Space
    kbd_register_key(&sys->kbd, 0x0F, 7, 1, 0); // SymShift
    kbd_register_key(&sys->kbd, 0x08, 3, 4, 1); // Cursor Left (Shift+5)
    kbd_register_key(&sys->kbd, 0x0A, 4, 4, 1); // Cursor Down (Shift+6)
    kbd_register_key(&sys->kbd, 0x0B, 4, 3, 1); // Cursor Up (Shift+7)
    kbd_register_key(&sys->kbd, 0x09, 4, 2, 1); // Cursor Right (Shift+8)
    kbd_register_key(&sys->kbd, 0x07, 3, 0, 1); // Edit (Shift+1)
    kbd_register_key(&sys->kbd, 0x0C, 4, 0, 1); // Delete (Shift+0)
    kbd_register_key(&sys->kbd, 0x0D, 6, 0, 0); // Enter
}

/*=== FILE LOADING ===========================================================*/

// ZX Z80 file format header (http://www.worldofspectrum.org/faq/reference/z80format.htm)
typedef struct {
    uint8_t A, F;
    uint8_t C, B;
    uint8_t L, H;
    uint8_t PC_l, PC_h;
    uint8_t SP_l, SP_h;
    uint8_t I, R;
    uint8_t flags0;
    uint8_t E, D;
    uint8_t C_, B_;
    uint8_t E_, D_;
    uint8_t L_, H_;
    uint8_t A_, F_;
    uint8_t IY_l, IY_h;
    uint8_t IX_l, IX_h;
    uint8_t EI;
    uint8_t IFF2;
    uint8_t flags1;
} _zx_z80_header;

typedef struct {
    uint8_t len_l;
    uint8_t len_h;
    uint8_t PC_l, PC_h;
    uint8_t hw_mode;
    uint8_t out_7ffd;
    uint8_t rom1;
    uint8_t flags;
    uint8_t out_fffd;
    uint8_t tlow_l;
    uint8_t tlow_h;
    uint8_t spectator_flags;
    uint8_t mgt_rom_paged;
    uint8_t multiface_rom_paged;
    uint8_t rom_0000_1fff;
    uint8_t rom_2000_3fff;
    uint8_t joy_mapping[10];
    uint8_t kbd_mapping[10];
    uint8_t mgt_type;
    uint8_t disciple_button_state;
    uint8_t disciple_flags;
    uint8_t out_1ffd;
} _zx_z80_ext_header;

typedef struct {
    uint8_t len_l;
    uint8_t len_h;
    uint8_t page_nr;
} _zx_z80_page_header;

static bool _zx_overflow(const uint8_t* ptr, intptr_t num_bytes, const uint8_t* end_ptr) {
    return (ptr + num_bytes) > end_ptr;
}

bool zx_quickload(zx_t* sys, chips_range_t data) {
    CHIPS_ASSERT(data.ptr && (data.size > 0));
    uint8_t* ptr = data.ptr;
    const uint8_t* end_ptr = ptr + data.size;
    if (_zx_overflow(ptr, sizeof(_zx_z80_header), end_ptr)) {
        return false;
    }
    const _zx_z80_header* hdr = (const _zx_z80_header*) ptr;
    ptr += sizeof(_zx_z80_header);
    const _zx_z80_ext_header* ext_hdr = 0;
    uint16_t pc = (hdr->PC_h<<8 | hdr->PC_l) & 0xFFFF;
    const bool is_version1 = 0 != pc;
    if (!is_version1) {
        if (_zx_overflow(ptr, sizeof(_zx_z80_ext_header), end_ptr)) {
            return false;
        }
        ext_hdr = (_zx_z80_ext_header*) ptr;
        int ext_hdr_len = (ext_hdr->len_h<<8)|ext_hdr->len_l;
        ptr += 2 + ext_hdr_len;
        if (ext_hdr->hw_mode >= 3) {
            return false;
        }
    }
    const bool v1_compr = 0 != (hdr->flags0 & (1<<5));
    while (ptr < end_ptr) {
        int page_index = 0;
        int src_len = 0;
        if (is_version1) {
            src_len = data.size - sizeof(_zx_z80_header);
        }
        else {
            _zx_z80_page_header* phdr = (_zx_z80_page_header*) ptr;
            if (_zx_overflow(ptr, sizeof(_zx_z80_page_header), end_ptr)) {
                return false;
            }
            ptr += sizeof(_zx_z80_page_header);
            src_len = (phdr->len_h<<8 | phdr->len_l) & 0xFFFF;
            page_index = phdr->page_nr - 3;
            if ((sys->type == ZX_TYPE_48K) && (page_index == 5)) {
                page_index = 0;
            }
            if ((page_index < 0) || (page_index > 7)) {
                page_index = -1;
            }
        }
        uint8_t* dst_ptr;
        if (-1 == page_index) {
            dst_ptr = NULL;
        } else {
            dst_ptr = sys->ram[page_index];
        }
        if (0xFFFF == src_len) {
            // FIXME: uncompressed not supported yet
            return false;
        }
        else {
            // compressed
            int src_pos = 0;
            bool v1_done = false;
            uint8_t val[4];
            while ((src_pos < src_len) && !v1_done) {
                val[0] = ptr[src_pos];
                val[1] = ptr[src_pos+1];
                val[2] = ptr[src_pos+2];
                val[3] = ptr[src_pos+3];
                // check for version 1 end marker
                if (v1_compr && (0==val[0]) && (0xED==val[1]) && (0xED==val[2]) && (0==val[3])) {
                    v1_done = true;
                    src_pos += 4;
                }
                else if (0xED == val[0]) {
                    if (0xED == val[1]) {
                        uint8_t count = val[2];
                        CHIPS_ASSERT(0 != count);
                        uint8_t data = val[3];
                        src_pos += 4;
                        for (int i = 0; i < count; i++) {
                            if (dst_ptr) *dst_ptr++ = data;
                        }
                    }
                    else {
                        // single ED
                        if (dst_ptr) *dst_ptr++ = val[0];
                        src_pos++;
                    }
                }
                else {
                    // any value
                    if (dst_ptr) *dst_ptr++ = val[0];
                    src_pos++;
                }
            }
            CHIPS_ASSERT(src_pos == src_len);
        }
        if (0xFFFF == src_len) {
            ptr += 0x4000;
        }
        else {
            ptr += src_len;
        }
    }

    // start loaded image
    z80_reset(&sys->cpu);
    sys->cpu.a = hdr->A; sys->cpu.f = hdr->F;
    sys->cpu.b = hdr->B; sys->cpu.c = hdr->C;
    sys->cpu.d = hdr->D; sys->cpu.e = hdr->E;
    sys->cpu.h = hdr->H; sys->cpu.l = hdr->L;
    sys->cpu.ix = (hdr->IX_h<<8)|hdr->IX_l;
    sys->cpu.iy = (hdr->IY_h<<8)|hdr->IY_l;
    sys->cpu.af2 = (hdr->A_<<8)|hdr->F_;
    sys->cpu.bc2 = (hdr->B_<<8)|hdr->C_;
    sys->cpu.de2 = (hdr->D_<<8)|hdr->E_;
    sys->cpu.hl2 = (hdr->H_<<8)|hdr->L_;
    sys->cpu.sp = (hdr->SP_h<<8)|hdr->SP_l;
    sys->cpu.i = hdr->I;
    sys->cpu.r = (hdr->R & 0x7F) | ((hdr->flags0 & 1)<<7);
    sys->cpu.iff2 = (hdr->IFF2 != 0);
    sys->cpu.iff1 = (hdr->EI != 0);
    if (hdr->flags1 != 0xFF) {
        sys->cpu.im = hdr->flags1 & 3;
    }
    else {
        sys->cpu.im = 1;
    }
    if (ext_hdr) {
        sys->pins = z80_prefetch(&sys->cpu, (ext_hdr->PC_h<<8)|ext_hdr->PC_l);
    }
    else {
        sys->pins = z80_prefetch(&sys->cpu, (hdr->PC_h<<8)|hdr->PC_l);
    }
    sys->border_color = (hdr->flags0>>1) & 7;
    return true;
}

chips_display_info_t zx_display_info(zx_t* sys) {
    static const uint32_t palette[16] = {
        0xFF000000,     // std black
        0xFFD70000,     // std blue
        0xFF0000D7,     // std red
        0xFFD700D7,     // std magenta
        0xFF00D700,     // std green
        0xFFD7D700,     // std cyan
        0xFF00D7D7,     // std yellow
        0xFFD7D7D7,     // std white
        0xFF000000,     // bright black
        0xFFFF0000,     // bright blue
        0xFF0000FF,     // bright red
        0xFFFF00FF,     // bright magenta
        0xFF00FF00,     // bright green
        0xFFFFFF00,     // bright cyan
        0xFF00FFFF,     // bright yellow
        0xFFFFFFFF,     // bright white
    };
    const chips_display_info_t res = {
        .frame = {
            .dim = {
                .width = ZX_FRAMEBUFFER_WIDTH,
                .height = ZX_FRAMEBUFFER_HEIGHT,
            },
            .bytes_per_pixel = 1,
        },
        .screen = {
            .x = 0,
            .y = 0,
            .width = ZX_DISPLAY_WIDTH,
            .height = ZX_DISPLAY_HEIGHT,
        },
        .palette = {
            .ptr = (void*)palette,
            .size = sizeof(palette),
        }
    };
    CHIPS_ASSERT(((sys == 0) && (res.frame.buffer.ptr == 0)) || ((sys != 0) && (res.frame.buffer.ptr != 0)));
    return res;
}

uint32_t zx_save_snapshot(zx_t* sys, zx_t* dst) {
    CHIPS_ASSERT(sys && dst);
    *dst = *sys;
    mem_snapshot_onsave(&dst->mem, sys);
    return ZX_SNAPSHOT_VERSION;
}

bool zx_load_snapshot(zx_t* sys, uint32_t version, zx_t* src) {
    CHIPS_ASSERT(sys && src);
    if (version != ZX_SNAPSHOT_VERSION) {
        return false;
    }
    /* Modified: use src in-place instead of a 65KB static copy */
    mem_snapshot_onload(&src->mem, sys);
    *sys = *src;
    return true;
}

#endif // CHIPS_IMPL
