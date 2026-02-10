/*
 * FRANK OS
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PSRAM_H
#define PSRAM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Uncached XIP window — ALL PSRAM access (detection + allocator) goes here.
 * The cached window (0x11000000) must NOT be used: it pollutes the XIP
 * cache with CS1 entries, causing QMI bus contention between CS0 flash
 * instruction fetches and CS1 PSRAM traffic that corrupts flash reads. */
#define PSRAM_BASE  0x15000000
#define PSRAM_END   0x15800000  /* base + 8MB max */

/* Detect PSRAM size by probing at power-of-two boundaries.
 * Returns detected size in bytes, or 0 if no PSRAM present. */
unsigned int butter_psram_size(void);

/* Initialize the PSRAM free-list allocator.
 * Call once at boot after psram_init() (if PSRAM HW init was done). */
void psram_heap_init(void);

/* Allocate size bytes from PSRAM.  Returns NULL if PSRAM is absent or full. */
void *psram_alloc(size_t size);

/* Free a pointer.  If ptr is in the PSRAM range, returns it to the PSRAM
 * free list; otherwise falls through to vPortFree(). */
void psram_free(void *ptr);

/* Returns true if PSRAM was detected and the allocator is active. */
bool psram_is_available(void);

/* Returns total detected PSRAM in bytes (cached after first probe). */
unsigned int psram_detected_bytes(void);

#endif
