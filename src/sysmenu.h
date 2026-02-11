/*
 * FRANK OS
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SYSMENU_H
#define SYSMENU_H

#include <stdint.h>
#include <stdbool.h>
#include "window.h"

/* Open the system menu for the focused window (Alt+Space) */
void sysmenu_open(hwnd_t hwnd);

/* Close the system menu */
void sysmenu_close(void);

/* Is the system menu open? */
bool sysmenu_is_open(void);

/* Draw the system menu as a compositor overlay */
void sysmenu_draw(void);

/* Handle mouse event. Returns true if consumed. */
bool sysmenu_mouse(uint8_t type, int16_t x, int16_t y);

/* Handle keyboard. Returns true if consumed. */
bool sysmenu_handle_key(uint8_t hid_code, uint8_t modifiers);

#endif /* SYSMENU_H */
