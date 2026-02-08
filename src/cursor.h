#ifndef CURSOR_H
#define CURSOR_H

#include <stdint.h>

/* Draw the mouse cursor at screen position (x, y).
 * Hotspot is at (0, 0) — top-left tip of the arrow. */
void cursor_draw(int16_t x, int16_t y);

#endif /* CURSOR_H */
