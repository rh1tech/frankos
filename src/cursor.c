#include "cursor.h"
#include "display.h"
#include "window.h" /* COLOR_BLACK, COLOR_WHITE */

#define CURSOR_W  12
#define CURSOR_HGT 19

/* 0 = transparent, 1 = black (outline), 2 = white (fill) */
static const uint8_t cursor_bitmap[CURSOR_HGT][CURSOR_W] = {
    { 1,0,0,0,0,0,0,0,0,0,0,0 },
    { 1,1,0,0,0,0,0,0,0,0,0,0 },
    { 1,2,1,0,0,0,0,0,0,0,0,0 },
    { 1,2,2,1,0,0,0,0,0,0,0,0 },
    { 1,2,2,2,1,0,0,0,0,0,0,0 },
    { 1,2,2,2,2,1,0,0,0,0,0,0 },
    { 1,2,2,2,2,2,1,0,0,0,0,0 },
    { 1,2,2,2,2,2,2,1,0,0,0,0 },
    { 1,2,2,2,2,2,2,2,1,0,0,0 },
    { 1,2,2,2,2,2,2,2,2,1,0,0 },
    { 1,2,2,2,2,2,2,2,2,2,1,0 },
    { 1,2,2,2,2,2,2,2,2,2,2,1 },
    { 1,2,2,2,2,2,2,1,1,1,1,1 },
    { 1,2,2,2,1,2,2,1,0,0,0,0 },
    { 1,2,2,1,0,1,2,2,1,0,0,0 },
    { 1,2,1,0,0,1,2,2,1,0,0,0 },
    { 1,1,0,0,0,0,1,2,2,1,0,0 },
    { 1,0,0,0,0,0,1,2,2,1,0,0 },
    { 0,0,0,0,0,0,0,1,1,0,0,0 },
};

void cursor_draw(int16_t x, int16_t y) {
    for (int row = 0; row < CURSOR_HGT; row++) {
        int py = y + row;
        if (py < 0 || py >= DISPLAY_HEIGHT) continue;
        for (int col = 0; col < CURSOR_W; col++) {
            uint8_t px_val = cursor_bitmap[row][col];
            if (px_val == 0) continue; /* transparent */
            int px = x + col;
            if (px < 0 || px >= DISPLAY_WIDTH) continue;
            display_set_pixel(px, py,
                              (px_val == 1) ? COLOR_BLACK : COLOR_WHITE);
        }
    }
}
