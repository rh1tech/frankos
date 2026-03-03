/*
 * FRANK OS — Windows ICO file parser
 * Copyright (c) 2026 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * Parses 4bpp Windows .ico files at runtime, extracting 16x16 and 32x32
 * icons with palette remapping (Windows -> CGA) and AND mask transparency.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ICO_H
#define ICO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Parse an ICO file and extract a 16x16 icon (256 bytes).
 * If no 16x16 entry exists, downscales from 32x32.
 * Returns true on success, false if the file is invalid or has no usable entry. */
bool ico_parse_16(const uint8_t *data, size_t len, uint8_t out[256]);

/* Parse an ICO file and extract a 32x32 icon (1024 bytes).
 * If no 32x32 entry exists, upscales from 16x16.
 * Returns true on success, false if the file is invalid or has no usable entry. */
bool ico_parse_32(const uint8_t *data, size_t len, uint8_t out[1024]);

#endif /* ICO_H */
