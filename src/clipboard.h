/*
 * FRANK OS — System Clipboard
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * Simple text clipboard backed by a 4KB static buffer in SRAM.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <stdint.h>
#include <stdbool.h>

#define CLIPBOARD_MAX_SIZE  4096

/* Copy text to the clipboard. len bytes are copied (no NUL required).
 * Returns true on success, false if len > CLIPBOARD_MAX_SIZE. */
bool clipboard_set_text(const char *text, uint16_t len);

/* Get pointer to current clipboard text (NUL-terminated).
 * Returns empty string "" if clipboard is empty. */
const char *clipboard_get_text(void);

/* Get length of current clipboard text (excluding NUL). */
uint16_t clipboard_get_length(void);

/* Clear the clipboard. */
void clipboard_clear(void);

#endif /* CLIPBOARD_H */
