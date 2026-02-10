/*
 * FRANK OS
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PS2_H
#define PS2_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/pio.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Mouse State
//=============================================================================

typedef struct {
    int16_t delta_x;      // Accumulated X movement
    int16_t delta_y;      // Accumulated Y movement
    int8_t  wheel;        // Accumulated wheel movement
    uint8_t buttons;      // Button state (bit 0=left, bit 1=right, bit 2=middle)
    uint8_t has_wheel;    // Non-zero if IntelliMouse detected
    uint8_t initialized;  // Non-zero if mouse init succeeded
} ps2_mouse_state_t;

//=============================================================================
// Initialization
//=============================================================================

/**
 * Initialize the unified PS/2 driver with both keyboard and mouse.
 * Sets up PIO program and claims state machines for keyboard and mouse.
 * 
 * @param pio        PIO instance to use (pio0 or pio1)
 * @param kbd_clk    Keyboard clock pin (data must be kbd_clk + 1)
 * @param mouse_clk  Mouse clock pin (data must be mouse_clk + 1)
 * @return true if initialization succeeded
 */
bool ps2_init(PIO pio, uint kbd_clk, uint mouse_clk);

/**
 * Initialize mouse only using PIO.
 * Use this when keyboard is managed by a separate driver.
 * 
 * @param pio        PIO instance to use (pio0 or pio1)
 * @param mouse_clk  Mouse clock pin (data must be mouse_clk + 1)
 * @return true if initialization succeeded
 */
bool ps2_mouse_pio_init(PIO pio, uint mouse_clk);

//=============================================================================
// Mouse API
//=============================================================================

/**
 * Initialize the mouse device (reset, detect type, enable streaming).
 * Must be called after ps2_init().
 * 
 * @return true if mouse responded and is ready
 */
bool ps2_mouse_init_device(void);

/**
 * Poll for mouse data. Call this frequently from main loop.
 */
void ps2_mouse_poll(void);

/**
 * Get accumulated mouse state and reset deltas.
 * 
 * @param dx      Output: X movement since last call
 * @param dy      Output: Y movement since last call  
 * @param wheel   Output: Wheel movement since last call
 * @param buttons Output: Current button state
 * @return true if there was any movement
 */
bool ps2_mouse_get_state(int16_t *dx, int16_t *dy, int8_t *wheel, uint8_t *buttons);

/**
 * Check if mouse is initialized and working.
 */
bool ps2_mouse_is_initialized(void);

/**
 * Check if mouse has scroll wheel (IntelliMouse).
 */
bool ps2_mouse_has_wheel(void);

/**
 * Get error statistics for debugging.
 */
void ps2_mouse_get_errors(uint32_t *frame_err, uint32_t *parity_err, uint32_t *sync_err);

//=============================================================================
// Keyboard API (raw byte access - higher level in ps2kbd)
//=============================================================================

/**
 * Check if keyboard has data available.
 */
bool ps2_kbd_has_data(void);

/**
 * Get next keyboard byte from FIFO.
 * @return byte value (0-255) or -1 if no data
 */
int ps2_kbd_get_byte(void);

/**
 * Get raw 22-bit frame from keyboard PIO FIFO.
 * @return raw frame or 0 if no data
 */
uint32_t ps2_kbd_get_raw(void);

#ifdef __cplusplus
}
#endif

#endif // PS2_H
