/*
 * FRANK OS — Find/Replace Dialog
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * Modeless Find and Find/Replace dialogs (Win95 Notepad style).
 * Results posted as WM_COMMAND to parent window.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FIND_DIALOG_H
#define FIND_DIALOG_H

#include <stdint.h>
#include <stdbool.h>
#include "window.h"

/* Result IDs posted as WM_COMMAND to parent */
#define DLG_RESULT_FIND_NEXT    0xFF31
#define DLG_RESULT_REPLACE      0xFF32
#define DLG_RESULT_REPLACE_ALL  0xFF33

/* Show a modeless Find dialog.
 * parent - owner window (receives WM_COMMAND with results)
 * Returns dialog hwnd (HWND_NULL on failure). */
hwnd_t find_dialog_show(hwnd_t parent);

/* Show a modeless Replace dialog.
 * parent - owner window (receives WM_COMMAND with results)
 * Returns dialog hwnd (HWND_NULL on failure). */
hwnd_t replace_dialog_show(hwnd_t parent);

/* Get the current search text. */
const char *find_dialog_get_text(void);

/* Get the current replacement text (Replace mode only). */
const char *find_dialog_get_replace_text(void);

/* Get current Match Case setting. */
bool find_dialog_case_sensitive(void);

/* Close the find/replace dialog. */
void find_dialog_close(void);

#endif /* FIND_DIALOG_H */
