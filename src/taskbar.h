/*
 * FRANK OS
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TASKBAR_H
#define TASKBAR_H

#include <stdint.h>
#include <stdbool.h>
#include "display.h"

#define TASKBAR_HEIGHT    28
#define TASKBAR_Y         (DISPLAY_HEIGHT - TASKBAR_HEIGHT)
#define TASKBAR_START_W   54

/* Initialize taskbar state */
void taskbar_init(void);

/* Draw the taskbar as a compositor overlay */
void taskbar_draw(void);

/* Handle mouse click in taskbar area.
 * Returns true if the click was in the taskbar. */
bool taskbar_mouse_click(int16_t x, int16_t y);

/* Get the usable work area height (screen minus taskbar) */
int16_t taskbar_work_area_height(void);

/* Force taskbar repaint (call when windows are created/destroyed/minimized) */
void taskbar_invalidate(void);

#endif /* TASKBAR_H */
