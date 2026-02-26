/*
 * FRANK OS — System Clipboard
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "clipboard.h"
#include <string.h>

static char     clip_buf[CLIPBOARD_MAX_SIZE + 1]; /* +1 for NUL */
static uint16_t clip_len;

bool clipboard_set_text(const char *text, uint16_t len) {
    if (len > CLIPBOARD_MAX_SIZE) return false;
    memcpy(clip_buf, text, len);
    clip_buf[len] = '\0';
    clip_len = len;
    return true;
}

const char *clipboard_get_text(void) {
    return clip_buf;
}

uint16_t clipboard_get_length(void) {
    return clip_len;
}

void clipboard_clear(void) {
    clip_buf[0] = '\0';
    clip_len = 0;
}
