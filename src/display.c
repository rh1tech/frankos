#include "display.h"
#include "hdmi.h"
#include <string.h>
#include <stdio.h>

// CGA/EGA 16-color palette (RGB888)
static const uint32_t default_palette[16] = {
    0x000000, // 0  Black
    0x0000AA, // 1  Blue
    0x00AA00, // 2  Green
    0x00AAAA, // 3  Cyan
    0xAA0000, // 4  Red
    0xAA00AA, // 5  Magenta
    0xAA5500, // 6  Brown
    0xAAAAAA, // 7  Light Gray
    0x555555, // 8  Dark Gray
    0x5555FF, // 9  Light Blue
    0x55FF55, // 10 Light Green
    0x55FFFF, // 11 Light Cyan
    0xFF5555, // 12 Light Red
    0xFF55FF, // 13 Light Magenta
    0xFFFF55, // 14 Yellow
    0xFFFFFF, // 15 White
};

// Double-buffered framebuffers (320 × 240, pair-encoded)
static uint8_t framebuffer_a[FB_STRIDE * FB_HEIGHT];
static uint8_t framebuffer_b[FB_STRIDE * FB_HEIGHT];

static uint8_t *draw_buffer = framebuffer_b;
static uint8_t *show_buffer = framebuffer_a;

void display_init(void) {
    memset(framebuffer_a, 0, sizeof(framebuffer_a));
    memset(framebuffer_b, 0, sizeof(framebuffer_b));

    // Init HDMI hardware, defer IRQ to Core 1
    graphics_init(true);

    // Set up pair-encoded palette
    graphics_init_pair_palette(default_palette);

    // Point HDMI at the show buffer
    graphics_set_buffer(show_buffer);
}

// Set pixel in the draw buffer (pair-encoded: 2 pixels per byte)
void display_set_pixel(int x, int y, uint8_t color) {
    if ((unsigned)x >= DISPLAY_WIDTH || (unsigned)y >= FB_HEIGHT) return;
    color &= 0x0F;
    uint8_t *p = &draw_buffer[y * FB_STRIDE + (x >> 1)];
    if (x & 1)
        *p = (*p & 0xF0) | color;         // right pixel = low nibble
    else
        *p = (*p & 0x0F) | (color << 4);  // left pixel = high nibble
}

void display_clear(uint8_t color) {
    uint8_t fill = (color << 4) | (color & 0x0F);
    memset(draw_buffer, fill, FB_STRIDE * FB_HEIGHT);
}

void display_swap_buffers(void) {
    uint8_t *tmp = draw_buffer;
    draw_buffer = show_buffer;
    show_buffer = tmp;
    graphics_request_buffer_swap(show_buffer);
}

void display_draw_test_pattern(void) {
    // 16 vertical color bars, each 20 bytes wide (40 pixels in 640 mode)
    // Each byte has identical left/right nibbles: (color << 4) | color
    for (int y = 0; y < FB_HEIGHT; y++) {
        uint8_t *row = &draw_buffer[y * FB_STRIDE];
        for (int bar = 0; bar < 16; bar++) {
            uint8_t fill = (bar << 4) | bar;
            memset(&row[bar * 20], fill, 20);
        }
    }
    printf("Test pattern: 16 color bars (640x480, pair-encoded)\n");
}
