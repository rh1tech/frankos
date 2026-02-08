#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#define DISPLAY_WIDTH  640
#define DISPLAY_HEIGHT 480
#define FB_WIDTH       320
#define FB_HEIGHT      480
#define FB_STRIDE      320
#define NUM_COLORS     16

void display_init(void);
void display_set_pixel(int x, int y, uint8_t color);
void display_clear(uint8_t color);
void display_swap_buffers(void);
void display_draw_test_pattern(void);

#endif
