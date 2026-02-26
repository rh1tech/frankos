/*
 * FRANK OS
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include "window.h"

/* Result IDs posted as WM_COMMAND to parent */
#define DLG_RESULT_FILE       0xFF20
#define DLG_RESULT_FILE_SAVE  0xFF21

/* Open a modal file browser dialog (Open mode).
 * parent       - owner window (receives WM_COMMAND with result)
 * title        - dialog title bar text
 * initial_path - starting directory (e.g., "/fos"), NULL defaults to "/"
 * filter_ext   - extension filter including dot (e.g., ".tap"), NULL = all files
 * Returns dialog hwnd (HWND_NULL on failure). */
hwnd_t file_dialog_open(hwnd_t parent, const char *title,
                        const char *initial_path, const char *filter_ext);

/* Open a modal file browser dialog (Save mode).
 * Same as file_dialog_open but with editable filename field.
 * initial_name - default filename (e.g., "untitled.txt"), NULL for empty.
 * Posts DLG_RESULT_FILE_SAVE on confirm. */
hwnd_t file_dialog_save(hwnd_t parent, const char *title,
                        const char *initial_path, const char *filter_ext,
                        const char *initial_name);

/* Get the full path of the selected/entered file.
 * Valid after DLG_RESULT_FILE or DLG_RESULT_FILE_SAVE is received. */
const char *file_dialog_get_path(void);

#endif /* FILE_DIALOG_H */
