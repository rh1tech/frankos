/*
 * FRANK OS — Alt+Tab Window Switcher Overlay
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ALTTAB_H
#define ALTTAB_H

#include "window.h"
#include <stdbool.h>
#include <stdint.h>

/* Open the Alt+Tab overlay.  Called on the first Alt+Tab press.
 * Builds the window list from the current z-stack and highlights
 * the second entry (the window you're switching TO). */
void alttab_open(void);

/* Cycle to the next entry in the overlay.  Called on subsequent
 * Tab presses while Alt is held.  Wraps around. */
void alttab_cycle(void);

/* Commit: switch focus to the currently highlighted window,
 * close the overlay, and resume/restore the target if needed.
 * Called when Alt is released. */
void alttab_commit(void);

/* Cancel: close the overlay without changing focus.
 * Called when Escape is pressed while the overlay is open. */
void alttab_cancel(void);

/* Draw the overlay.  Called from the compositor after menus. */
void alttab_draw(void);

/* Query whether the overlay is currently active. */
bool alttab_is_active(void);

#endif /* ALTTAB_H */
