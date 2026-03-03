/*
 * FRANK OS — Desktop shortcut manager
 * Copyright (c) 2026 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * Desktop displays shortcut icons for files and applications.
 * Shortcuts are added from Navigator via "Send to Desktop" and
 * persisted in /fos/desktop.dat.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DESKTOP_H
#define DESKTOP_H

#include <stdint.h>
#include <stdbool.h>

/*==========================================================================
 * Constants
 *=========================================================================*/

#define DESKTOP_MAX_SHORTCUTS  24
#define DESKTOP_PATH_MAX       128
#define DESKTOP_NAME_MAX       20
#define DESKTOP_ICON16_SIZE    256   /* 16x16 palette-index (in .inf) */
#define DESKTOP_ICON32_SIZE    1024  /* 32x32 palette-index (in .inf) */
#define DESKTOP_ICON_SIZE      DESKTOP_ICON32_SIZE  /* shortcut slot size */

#define DESKTOP_DAT_PATH       "/fos/desktop.dat"

/* Special paths for built-in apps (not real filesystem paths) */
#define DESKTOP_BUILTIN_NAVIGATOR  ":navigator"
#define DESKTOP_BUILTIN_TERMINAL   ":terminal"

/*==========================================================================
 * Context-menu command IDs
 *=========================================================================*/

#define DT_CMD_OPEN             600
#define DT_CMD_REMOVE           601
#define DT_CMD_SORT_NAME        602
#define DT_CMD_REFRESH          603

/*==========================================================================
 * API
 *=========================================================================*/

/* Initialize the desktop — load shortcuts from desktop.dat */
void desktop_init(void);

/* Paint desktop icons in the work area (called by compositor) */
void desktop_paint(void);

/* Add a shortcut for a file or app path.
 * Determines icon and name automatically. */
bool desktop_add_shortcut(const char *path);

/* Remove a shortcut by index */
void desktop_remove_shortcut(int idx);

/* Sort shortcuts alphabetically */
void desktop_sort(void);

/* Handle mouse events on the desktop area.
 * Returns true if the event was consumed. */
bool desktop_mouse(uint8_t type, int16_t x, int16_t y);

/* Handle the right-click context menu result */
bool desktop_handle_command(uint16_t cmd);

/* Check if the desktop needs repainting */
bool desktop_is_dirty(void);

/* Mark the desktop for repainting */
void desktop_mark_dirty(void);

/* True if there are desktop shortcuts */
bool desktop_has_shortcuts(void);

/* Get the 16x16 icon of the first shortcut (for Alt+Tab display).
 * Returns default_icon_16x16 if no shortcuts or no icon. */
const uint8_t *desktop_get_icon(void);

/* Get a 32x32 desktop icon (loaded from /fos/desktop.ico if available,
 * otherwise upscaled from desktop_icon_16x16).
 * The returned pointer is to a static buffer — valid until next call. */
const uint8_t *desktop_get_icon32(void);

/* Give keyboard focus to the desktop (selects first shortcut).
 * Called when "Desktop" is chosen from Alt+Tab. */
void desktop_focus(void);

/* Returns true if the desktop currently has keyboard focus. */
bool desktop_has_focus(void);

/* Handle a keyboard event while the desktop has focus.
 * Returns true if the event was consumed. */
bool desktop_key(uint8_t scancode, uint8_t modifiers);

#endif /* DESKTOP_H */
