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

/* Set taskbar dirty without calling wm_mark_dirty (for compositor use) */
void taskbar_force_dirty(void);

/* Query whether taskbar needs repainting this frame */
bool taskbar_needs_redraw(void);

/* Right-click context menu on taskbar buttons */
bool taskbar_mouse_rclick(int16_t x, int16_t y);
void taskbar_popup_draw(void);
bool taskbar_popup_mouse(uint8_t type, int16_t x, int16_t y);
bool taskbar_popup_handle_key(uint8_t hid_code, uint8_t modifiers);
bool taskbar_popup_is_open(void);
void taskbar_popup_close(void);

#endif /* TASKBAR_H */
