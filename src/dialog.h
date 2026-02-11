/*
 * FRANK OS
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DIALOG_H
#define DIALOG_H

#include <stdint.h>
#include "window.h"

/* Icon types */
#define DLG_ICON_NONE       0
#define DLG_ICON_INFO       1
#define DLG_ICON_WARNING    2
#define DLG_ICON_ERROR      3

/* Button configurations (OR together) */
#define DLG_BTN_OK          (1u << 0)
#define DLG_BTN_CANCEL      (1u << 1)
#define DLG_BTN_YES         (1u << 2)
#define DLG_BTN_NO          (1u << 3)

/* Result IDs (posted as WM_COMMAND to parent).
 * High values to avoid collision with menu command IDs. */
#define DLG_RESULT_OK       0xFF01
#define DLG_RESULT_CANCEL   0xFF02
#define DLG_RESULT_YES      0xFF03
#define DLG_RESULT_NO       0xFF04

/* Show a modal dialog. Returns dialog hwnd (HWND_NULL on failure).
 * parent  - owner window (receives WM_COMMAND with result, 0 = no notification)
 * title   - dialog title bar text
 * text    - body text (supports \n for line breaks)
 * icon    - DLG_ICON_* constant
 * buttons - DLG_BTN_* flags OR'd together */
hwnd_t dialog_show(hwnd_t parent, const char *title, const char *text,
                   uint8_t icon, uint8_t buttons);

#endif /* DIALOG_H */
