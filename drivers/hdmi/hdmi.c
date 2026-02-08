#include "../../src/board_config.h"
#include "hdmi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdalign.h>
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/time.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "pico/platform.h"

// Flag to defer IRQ handler setup to Core 1
static bool g_defer_irq_to_core1 = false;

// Framebuffer: 320 bytes per line (pair-encoded: each byte = 2 pixels = 640 px)
// 240 lines with vertical doubling to fill 480 scanlines
#define SCREEN_WIDTH  (320)
#define SCREEN_HEIGHT (480)

static uint8_t *graphics_buffer = NULL;
static volatile uint8_t *graphics_pending_buffer = NULL;
static volatile uint32_t graphics_frame_count = 0;
static volatile bool graphics_swap_done = false;

/* Snapshot of graphics_buffer taken at vsync — used by the IRQ handler
 * throughout the frame so a mid-frame swap request can never cause the
 * handler to read from two different buffers in the same frame. */
static uint8_t *scanline_source = NULL;

void graphics_set_buffer(uint8_t *buffer) {
    graphics_buffer = buffer;
    scanline_source = buffer;
}

void graphics_request_buffer_swap(uint8_t *buffer) {
    graphics_swap_done = false;
    graphics_pending_buffer = buffer;
}

bool graphics_swap_completed(void) {
    return graphics_swap_done;
}

uint32_t __not_in_flash() get_frame_count(void) {
    return graphics_frame_count;
}

static inline uint8_t* __not_in_flash_func(get_line_buffer)(int line) {
    if (!graphics_buffer) return NULL;
    if (line < 0 || line >= SCREEN_HEIGHT) return NULL;
    return graphics_buffer + line * SCREEN_WIDTH;
}

static struct video_mode_t video_mode = {
    // 640x480 60Hz
    .h_total = 524,
    .h_width = 480,
    .freq = 60,
    .vgaPxClk = 25175000
};

void __not_in_flash_func(vsync_handler)() {
    graphics_frame_count++;
    uint8_t *pending = (uint8_t *)graphics_pending_buffer;
    if (pending) {
        graphics_buffer = pending;
        graphics_pending_buffer = NULL;
        graphics_swap_done = true;
    }
    /* Snapshot the active buffer for this entire frame — the IRQ handler
     * must never see a buffer pointer change mid-frame. */
    scanline_source = graphics_buffer;
}

// --- PIO and DMA state ---

static uint offs_prg0 = 0;
static uint offs_prg1 = 0;
static int SM_video = -1;
static int SM_conv = -1;

/*
 * Sync-control palette index base.
 *
 * The pair-encoded framebuffer uses all 256 byte values (16×16 color pairs).
 * Entries BASE through BASE+3 are overwritten with HDMI sync/control tokens
 * by write_sync_control_entries(), so those 4 byte values MUST NOT appear in
 * the active video area of the framebuffer.
 *
 * Byte value = (left_nibble << 4) | right_nibble.  Choosing BASE = 208
 * (0xD0-0xD3) sacrifices the pairs (13,0), (13,1), (13,2), (13,3) —
 * i.e. light-magenta paired with black/blue/green/cyan.
 *
 * Previously BASE was 240, which collided with (15,0)-(15,3) = white paired
 * with black/blue/green/cyan — extremely common in desktop UI (white text on
 * blue title bar produces byte 0xF1 = 241).
 */
#define BASE_HDMI_CTRL_INX (208)

// DMA channels
static int dma_chan_ctrl;
static int dma_chan;
static int dma_chan_pal_conv_ctrl;
static int dma_chan_pal_conv;
static int dma_chan_copy = -1;  // DMA channel for memcpy

// DMA line buffers
static uint32_t* dma_lines[2] = { NULL, NULL };
static uint32_t* DMA_BUF_ADDR[2];

// Copy DMA config
static dma_channel_config dma_conf_copy;

// Palette conversion table — 4096-byte aligned
// 256 entries × 2 uint64_t (left+right pixel TMDS) = 1024 uint32_t
// Plus 208 uint32_t for two DMA line buffers at the tail (104 each).
// Each line buffer is 408 bytes: 400 real + 8 padding blanking bytes.
// The padding keeps the DMA pipeline fed during the chain restart gap
// between scanlines, preventing the video PIO FIFO from underrunning.
alignas(4096) uint32_t conv_color[1232];

static uint32_t irq_inx = 0;

// --- PIO Programs ---

// Address converter PIO program
static uint16_t pio_program_instructions_conv_HDMI[] = {
    0x80a0, //  0: pull   block
    0x40e8, //  1: in     osr, 8
    0x4034, //  2: in     x, 20
    0x8020, //  3: push   block
};

static const struct pio_program pio_program_conv_addr_HDMI = {
    .instructions = pio_program_instructions_conv_HDMI,
    .length = 4,
    .origin = -1,
};

// Video output PIO program
static const uint16_t instructions_PIO_HDMI[] = {
    0x7006, //  0: out    pins, 6         side 2
    0x7006, //  1: out    pins, 6         side 2
    0x7006, //  2: out    pins, 6         side 2
    0x7006, //  3: out    pins, 6         side 2
    0x7006, //  4: out    pins, 6         side 2
    0x6806, //  5: out    pins, 6         side 1
    0x6806, //  6: out    pins, 6         side 1
    0x6806, //  7: out    pins, 6         side 1
    0x6806, //  8: out    pins, 6         side 1
    0x6806, //  9: out    pins, 6         side 1
};

static const struct pio_program program_PIO_HDMI = {
    .instructions = instructions_PIO_HDMI,
    .length = 10,
    .origin = -1,
};

// --- TMDS encoder ---

static uint64_t get_ser_diff_data(const uint16_t dataR, const uint16_t dataG, const uint16_t dataB) {
    uint64_t out64 = 0;
    for (int i = 0; i < 10; i++) {
        out64 <<= 6;
        if (i == 5) out64 <<= 2;

        uint8_t bR = (dataR >> (9 - i)) & 1;
        uint8_t bG = (dataG >> (9 - i)) & 1;
        uint8_t bB = (dataB >> (9 - i)) & 1;

        bR |= (bR ^ 1) << 1;
        bG |= (bG ^ 1) << 1;
        bB |= (bB ^ 1) << 1;

        if (HDMI_PIN_invert_diffpairs) {
            bR ^= 0b11;
            bG ^= 0b11;
            bB ^= 0b11;
        }
        uint8_t d6;
        if (HDMI_PIN_RGB_notBGR) {
            d6 = (bR << 4) | (bG << 2) | (bB << 0);
        } else {
            d6 = (bB << 4) | (bG << 2) | (bR << 0);
        }

        out64 |= d6;
    }
    return out64;
}

static uint tmds_encoder(const uint8_t d8) {
    int s1 = 0;
    for (int i = 0; i < 8; i++) s1 += (d8 & (1 << i)) ? 1 : 0;
    bool is_xnor = false;
    if ((s1 > 4) || ((s1 == 4) && ((d8 & 1) == 0))) is_xnor = true;
    uint16_t d_out = d8 & 1;
    uint16_t qi = d_out;
    for (int i = 1; i < 8; i++) {
        d_out |= ((qi << 1) ^ (d8 & (1 << i))) ^ (is_xnor << i);
        qi = d_out & (1 << i);
    }
    if (is_xnor) d_out |= 1 << 9;
    else d_out |= 1 << 8;
    return d_out;
}

static void pio_set_x(PIO pio, const int sm, uint32_t v) {
    uint instr_shift = pio_encode_in(pio_x, 4);
    uint instr_mov = pio_encode_mov(pio_x, pio_isr);
    for (int i = 0; i < 8; i++) {
        const uint32_t nibble = (v >> (i * 4)) & 0xf;
        pio_sm_exec(pio, sm, pio_encode_set(pio_x, nibble));
        pio_sm_exec(pio, sm, instr_shift);
    }
    pio_sm_exec(pio, sm, instr_mov);
}

// --- Fast memcpy / memset (runs from SRAM) ---

static inline void* __not_in_flash_func(nf_memcpy)(void* dst, const void* src, size_t len) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;

    /* Align destination to 4-byte boundary */
    while (len && ((uintptr_t)d & 3)) {
        *d++ = *s++;
        len--;
    }

    if (((uintptr_t)s & 3) == 0) {
        /* Both aligned — copy words */
        uint32_t* d32 = (uint32_t*)d;
        const uint32_t* s32 = (const uint32_t*)s;
        size_t n32 = len >> 2;
        while (n32--) {
            *d32++ = *s32++;
        }
        d = (uint8_t*)d32;
        s = (const uint8_t*)s32;
        len &= 3;
    }

    while (len--) {
        *d++ = *s++;
    }

    return dst;
}

static inline void* __not_in_flash_func(nf_memset)(void* ptr, int value, size_t len) {
    uint8_t* p = (uint8_t*)ptr;
    uint8_t v8 = (uint8_t)value;

    while (len && ((uintptr_t)p & 3)) {
        *p++ = v8;
        len--;
    }

    if (len >= 4) {
        uint32_t v32 = v8;
        v32 |= v32 << 8;
        v32 |= v32 << 16;
        uint32_t* p32 = (uint32_t*)p;
        size_t n32 = len >> 2;
        while (n32--) {
            *p32++ = v32;
        }
        p = (uint8_t*)p32;
        len &= 3;
    }

    while (len--) {
        *p++ = v8;
    }

    return ptr;
}

// --- Scanline DMA IRQ handler ---

static void __not_in_flash_func(dma_handler_HDMI)() {
    static uint32_t inx_buf_dma;
    static uint line = 0;
    irq_inx++;

    dma_hw->ints0 = 1u << dma_chan_ctrl;
    dma_channel_set_read_addr(dma_chan_ctrl, &DMA_BUF_ADDR[(inx_buf_dma + 1) & 1], false);

    if (line >= video_mode.h_total) {
        line = 0;
        vsync_handler();
    } else {
        ++line;
    }

    inx_buf_dma++;

    uint8_t* activ_buf = (uint8_t *)dma_lines[inx_buf_dma & 1];

    if (line < video_mode.h_width) {
        uint8_t* output_buffer = activ_buf + 72; // sync alignment offset
        int y = line;  // 1:1 mapping, no vertical doubling

        /* Use the frame-stable snapshot pointer, never the live pointer */
        uint8_t *src = scanline_source;
        if (src && y >= 0 && y < SCREEN_HEIGHT) {
            nf_memcpy(output_buffer, src + y * SCREEN_WIDTH, SCREEN_WIDTH);
        } else {
            nf_memset(output_buffer, 0, SCREEN_WIDTH);
        }

        // Horizontal sync intervals
        nf_memset(activ_buf + 48, BASE_HDMI_CTRL_INX, 24);
        nf_memset(activ_buf, BASE_HDMI_CTRL_INX + 1, 48);
        nf_memset(activ_buf + 392, BASE_HDMI_CTRL_INX, 16); // 8 front porch + 8 padding
    } else {
        if ((line >= 490) && (line < 492)) {
            // Vertical sync pulse
            nf_memset(activ_buf + 48, BASE_HDMI_CTRL_INX + 2, 360); // 352 + 8 padding
            nf_memset(activ_buf, BASE_HDMI_CTRL_INX + 3, 48);
        } else {
            // Blanking (no image)
            nf_memset(activ_buf + 48, BASE_HDMI_CTRL_INX, 360); // 352 + 8 padding
            nf_memset(activ_buf, BASE_HDMI_CTRL_INX + 1, 48);
        }
    }
}

// --- IRQ helpers ---

static inline void irq_remove_handler_DMA_core1() {
    irq_set_enabled(VIDEO_DMA_IRQ, false);
    irq_handler_t handler = irq_get_exclusive_handler(VIDEO_DMA_IRQ);
    if (handler) {
        irq_remove_handler(VIDEO_DMA_IRQ, handler);
    }
}

static inline void irq_set_exclusive_handler_DMA_core1() {
    irq_set_exclusive_handler(VIDEO_DMA_IRQ, dma_handler_HDMI);
    irq_set_priority(VIDEO_DMA_IRQ, 0);
    irq_set_enabled(VIDEO_DMA_IRQ, true);
}

// --- Palette: set single color entry (internal) ---

static void graphics_set_palette_entry(uint8_t i, uint32_t color888) {
    // Skip sync control indices
    if (i >= BASE_HDMI_CTRL_INX && i <= BASE_HDMI_CTRL_INX + 3) return;

    uint64_t* conv_color64 = (uint64_t *)conv_color;
    const uint8_t R = (color888 >> 16) & 0xff;
    const uint8_t G = (color888 >> 8) & 0xff;
    const uint8_t B = (color888 >> 0) & 0xff;
    conv_color64[i * 2] = get_ser_diff_data(tmds_encoder(R), tmds_encoder(G), tmds_encoder(B));
    conv_color64[i * 2 + 1] = conv_color64[i * 2] ^ 0x0003ffffffffffffl;
}

// --- Sync control entries ---

static void write_sync_control_entries(void) {
    uint64_t* conv_color64 = (uint64_t *)conv_color;
    const uint16_t b0 = 0b1101010100;
    const uint16_t b1 = 0b0010101011;
    const uint16_t b2 = 0b0101010100;
    const uint16_t b3 = 0b1010101011;
    const int base_inx = BASE_HDMI_CTRL_INX;

    conv_color64[2 * base_inx + 0] = get_ser_diff_data(b0, b0, b3);
    conv_color64[2 * base_inx + 1] = get_ser_diff_data(b0, b0, b3);

    conv_color64[2 * (base_inx + 1) + 0] = get_ser_diff_data(b0, b0, b2);
    conv_color64[2 * (base_inx + 1) + 1] = get_ser_diff_data(b0, b0, b2);

    conv_color64[2 * (base_inx + 2) + 0] = get_ser_diff_data(b0, b0, b1);
    conv_color64[2 * (base_inx + 2) + 1] = get_ser_diff_data(b0, b0, b1);

    conv_color64[2 * (base_inx + 3) + 0] = get_ser_diff_data(b0, b0, b0);
    conv_color64[2 * (base_inx + 3) + 1] = get_ser_diff_data(b0, b0, b0);
}

// --- Pair-encoded palette initialization ---

void graphics_init_pair_palette(const uint32_t colors[16]) {
    uint64_t* conv_color64 = (uint64_t *)conv_color;

    // Pre-compute TMDS-encoded 64-bit values for each of the 16 colors
    uint64_t tmds_even[16]; // "even pixel" variant
    uint64_t tmds_odd[16];  // "odd pixel" variant (XOR'd)

    for (int c = 0; c < 16; c++) {
        uint8_t R = (colors[c] >> 16) & 0xff;
        uint8_t G = (colors[c] >> 8) & 0xff;
        uint8_t B = (colors[c] >> 0) & 0xff;
        tmds_even[c] = get_ser_diff_data(tmds_encoder(R), tmds_encoder(G), tmds_encoder(B));
        tmds_odd[c] = tmds_even[c] ^ 0x0003ffffffffffffl;
    }

    // Fill all 256 byte values: byte b encodes pixel pair (b>>4, b&0xF)
    // conv_color64[b*2+0] = left pixel (high nibble) — even position
    // conv_color64[b*2+1] = right pixel (low nibble) — odd position
    for (int b = 0; b < 256; b++) {
        int left = (b >> 4) & 0xF;
        int right = b & 0xF;
        conv_color64[b * 2 + 0] = tmds_even[left];
        conv_color64[b * 2 + 1] = tmds_odd[right];
    }

    // Overwrite sync control entries 240-243
    write_sync_control_entries();
}

// --- Hardware init/reinit ---

static inline bool hdmi_init(void) {
    static bool initialized = false;

    if (initialized) {
        // Cleanup for reinit — only safe after first successful init
        if (VIDEO_DMA_IRQ == DMA_IRQ_0) {
            dma_channel_set_irq0_enabled(dma_chan_ctrl, false);
        } else {
            dma_channel_set_irq1_enabled(dma_chan_ctrl, false);
        }

        irq_remove_handler_DMA_core1();

        // Stop all DMA channels
        dma_hw->abort = (1 << dma_chan_ctrl) | (1 << dma_chan) |
                        (1 << dma_chan_pal_conv) | (1 << dma_chan_pal_conv_ctrl) |
                        (1 << dma_chan_copy);
        while (dma_hw->abort) tight_loop_contents();

        // Disable PIO state machines
        pio_sm_set_enabled(PIO_VIDEO, SM_video, false);
        pio_sm_set_enabled(PIO_VIDEO_ADDR, SM_conv, false);

        // Remove old PIO programs
        pio_remove_program(PIO_VIDEO_ADDR, &pio_program_conv_addr_HDMI, offs_prg1);
        pio_remove_program(PIO_VIDEO, &program_PIO_HDMI, offs_prg0);
    }
    initialized = true;

    offs_prg1 = pio_add_program(PIO_VIDEO_ADDR, &pio_program_conv_addr_HDMI);
    offs_prg0 = pio_add_program(PIO_VIDEO, &program_PIO_HDMI);

    pio_set_x(PIO_VIDEO_ADDR, SM_conv, ((uint32_t)conv_color >> 12));

    // Sync control entries should already be set by graphics_init_pair_palette()
    // but ensure they're correct after reinit
    write_sync_control_entries();

    // Configure address-converter PIO SM
    pio_sm_config c_c = pio_get_default_sm_config();
    sm_config_set_wrap(&c_c, offs_prg1, offs_prg1 + (pio_program_conv_addr_HDMI.length - 1));
    sm_config_set_in_shift(&c_c, true, false, 32);

    pio_sm_init(PIO_VIDEO_ADDR, SM_conv, offs_prg1, &c_c);
    pio_sm_set_enabled(PIO_VIDEO_ADDR, SM_conv, true);

    // Configure video output PIO SM
    c_c = pio_get_default_sm_config();
    sm_config_set_wrap(&c_c, offs_prg0, offs_prg0 + (program_PIO_HDMI.length - 1));

    // Side-set for clock pins
    sm_config_set_sideset_pins(&c_c, beginHDMI_PIN_clk);
    sm_config_set_sideset(&c_c, 2, false, false);
    for (int i = 0; i < 2; i++) {
        pio_gpio_init(PIO_VIDEO, beginHDMI_PIN_clk + i);
        gpio_set_drive_strength(beginHDMI_PIN_clk + i, GPIO_DRIVE_STRENGTH_12MA);
        gpio_set_slew_rate(beginHDMI_PIN_clk + i, GPIO_SLEW_RATE_FAST);
    }

    pio_sm_set_pins_with_mask(PIO_VIDEO, SM_video, 3u << beginHDMI_PIN_clk, 3u << beginHDMI_PIN_clk);
    pio_sm_set_pindirs_with_mask(PIO_VIDEO, SM_video, 3u << beginHDMI_PIN_clk, 3u << beginHDMI_PIN_clk);

    // Data pins
    for (int i = 0; i < 6; i++) {
        gpio_set_slew_rate(beginHDMI_PIN_data + i, GPIO_SLEW_RATE_FAST);
        pio_gpio_init(PIO_VIDEO, beginHDMI_PIN_data + i);
        gpio_set_drive_strength(beginHDMI_PIN_data + i, GPIO_DRIVE_STRENGTH_12MA);
    }
    pio_sm_set_consecutive_pindirs(PIO_VIDEO, SM_video, beginHDMI_PIN_data, 6, true);
    sm_config_set_out_pins(&c_c, beginHDMI_PIN_data, 6);

    sm_config_set_out_shift(&c_c, true, true, 30);
    sm_config_set_fifo_join(&c_c, PIO_FIFO_JOIN_TX);

    int hdmi_hz = video_mode.freq;
    sm_config_set_clkdiv(&c_c, (clock_get_hz(clk_sys) / 252000000.0f) * (60.0f / hdmi_hz));
    pio_sm_init(PIO_VIDEO, SM_video, offs_prg0, &c_c);
    pio_sm_set_enabled(PIO_VIDEO, SM_video, true);

    // DMA line buffers sit at the tail of conv_color (104 words each)
    dma_lines[0] = &conv_color[1024];
    dma_lines[1] = &conv_color[1128];

    // Pre-fill both line buffers with blanking data so the very first
    // scanlines output valid HDMI sync instead of random TMDS garbage.
    for (int b = 0; b < 2; b++) {
        uint8_t *buf = (uint8_t *)dma_lines[b];
        nf_memset(buf + 48, BASE_HDMI_CTRL_INX, 360);
        nf_memset(buf, BASE_HDMI_CTRL_INX + 1, 48);
    }

    // Main scanline data DMA channel
    dma_channel_config cfg_dma = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_8);
    channel_config_set_chain_to(&cfg_dma, dma_chan_ctrl);
    channel_config_set_high_priority(&cfg_dma, true);
    channel_config_set_read_increment(&cfg_dma, true);
    channel_config_set_write_increment(&cfg_dma, false);

    uint dreq = DREQ_PIO1_TX0 + SM_conv;
    if (PIO_VIDEO_ADDR == pio0) dreq = DREQ_PIO0_TX0 + SM_conv;
    channel_config_set_dreq(&cfg_dma, dreq);

    dma_channel_configure(
        dma_chan, &cfg_dma,
        &PIO_VIDEO_ADDR->txf[SM_conv],
        &dma_lines[0][0],
        408,
        false
    );

    // Control channel for main DMA
    cfg_dma = dma_channel_get_default_config(dma_chan_ctrl);
    channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
    channel_config_set_chain_to(&cfg_dma, dma_chan);
    channel_config_set_high_priority(&cfg_dma, true);
    channel_config_set_read_increment(&cfg_dma, false);
    channel_config_set_write_increment(&cfg_dma, false);

    DMA_BUF_ADDR[0] = &dma_lines[0][0];
    DMA_BUF_ADDR[1] = &dma_lines[1][0];

    dma_channel_configure(
        dma_chan_ctrl, &cfg_dma,
        &dma_hw->ch[dma_chan].read_addr,
        &DMA_BUF_ADDR[0],
        1,
        false
    );

    // Palette converter DMA channel
    cfg_dma = dma_channel_get_default_config(dma_chan_pal_conv);
    channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
    channel_config_set_chain_to(&cfg_dma, dma_chan_pal_conv_ctrl);
    channel_config_set_high_priority(&cfg_dma, true);
    channel_config_set_read_increment(&cfg_dma, true);
    channel_config_set_write_increment(&cfg_dma, false);

    dreq = DREQ_PIO1_TX0 + SM_video;
    if (PIO_VIDEO == pio0) dreq = DREQ_PIO0_TX0 + SM_video;
    channel_config_set_dreq(&cfg_dma, dreq);

    dma_channel_configure(
        dma_chan_pal_conv, &cfg_dma,
        &PIO_VIDEO->txf[SM_video],
        &conv_color[0],
        4,
        false
    );

    // Palette converter control channel
    cfg_dma = dma_channel_get_default_config(dma_chan_pal_conv_ctrl);
    channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
    channel_config_set_chain_to(&cfg_dma, dma_chan_pal_conv);
    channel_config_set_high_priority(&cfg_dma, true);
    channel_config_set_read_increment(&cfg_dma, false);
    channel_config_set_write_increment(&cfg_dma, false);

    dreq = DREQ_PIO1_RX0 + SM_conv;
    if (PIO_VIDEO_ADDR == pio0) dreq = DREQ_PIO0_RX0 + SM_conv;
    channel_config_set_dreq(&cfg_dma, dreq);

    dma_channel_configure(
        dma_chan_pal_conv_ctrl, &cfg_dma,
        &dma_hw->ch[dma_chan_pal_conv].read_addr,
        &PIO_VIDEO_ADDR->rxf[SM_conv],
        1,
        true  // Start this one
    );

    // Enable DMA IRQ
    if (VIDEO_DMA_IRQ == DMA_IRQ_0) {
        dma_channel_acknowledge_irq0(dma_chan_ctrl);
        dma_channel_set_irq0_enabled(dma_chan_ctrl, true);
    } else {
        dma_channel_acknowledge_irq1(dma_chan_ctrl);
        dma_channel_set_irq1_enabled(dma_chan_ctrl, true);
    }

    if (!g_defer_irq_to_core1) {
        irq_set_exclusive_handler_DMA_core1();
        dma_start_channel_mask((1u << dma_chan_ctrl));
    }

    return true;
}

// --- Public API ---

void graphics_init(bool defer_irq) {
    g_defer_irq_to_core1 = defer_irq;

    // Claim PIO state machines and DMA channels
    SM_video = pio_claim_unused_sm(PIO_VIDEO, true);
    SM_conv = pio_claim_unused_sm(PIO_VIDEO_ADDR, true);
    dma_chan_ctrl = dma_claim_unused_channel(true);
    dma_chan = dma_claim_unused_channel(true);
    dma_chan_pal_conv_ctrl = dma_claim_unused_channel(true);
    dma_chan_pal_conv = dma_claim_unused_channel(true);
    dma_chan_copy = dma_claim_unused_channel(true);

    // One-time configuration for the copy channel config object
    dma_conf_copy = dma_channel_get_default_config(dma_chan_copy);
    channel_config_set_transfer_data_size(&dma_conf_copy, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_conf_copy, true);
    channel_config_set_write_increment(&dma_conf_copy, true);
    channel_config_set_high_priority(&dma_conf_copy, false); // Normal priority to yield to Video DMA

    printf("HDMI: SM_video=%u, SM_conv=%u\n", SM_video, SM_conv);
    printf("HDMI: DMA channels: ctrl=%u, data=%u, pal_ctrl=%u, pal=%u, copy=%u\n",
        dma_chan_ctrl, dma_chan, dma_chan_pal_conv_ctrl, dma_chan_pal_conv, dma_chan_copy);

    hdmi_init();

    // Configure the copy channel but don't start it yet
    dma_channel_configure(
        dma_chan_copy, &dma_conf_copy,
        NULL, // Write addr set later
        NULL, // Read addr set later
        SCREEN_WIDTH / 4,
        false // Don't start
    );

    printf("HDMI: Init complete\n");
}

void graphics_init_irq_on_this_core(void) {
    irq_set_exclusive_handler(VIDEO_DMA_IRQ, dma_handler_HDMI);
    irq_set_priority(VIDEO_DMA_IRQ, 0);
    irq_set_enabled(VIDEO_DMA_IRQ, true);

    if (g_defer_irq_to_core1) {
        dma_start_channel_mask((1u << dma_chan_ctrl));
    }

    printf("HDMI: IRQ handler set on core %u, DMA started\n", get_core_num());
}
