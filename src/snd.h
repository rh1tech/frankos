/*
 * FRANK OS — Sound Mixer
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * Multi-channel mixing with per-channel resampling.  DMA IRQ pulls from
 * ring buffers so buffered audio keeps playing even when tasks stall.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <stdint.h>

#define SND_MAX_CHANNELS  4
#define SND_SYSTEM_RATE   44100

/* Call once at boot — starts I2S, DMA plays silence */
void snd_init(void);

/* Open a sound channel.  Returns channel ID (0-3) or -1 if full.
 * sample_rate: source sample rate (e.g. 15625, 22050, 44100) */
int  snd_open(uint32_t sample_rate);

/* Write stereo interleaved frames to a channel.  Blocks if ring buffer full. */
void snd_write(int ch, const int16_t *samples, int frames);

/* Close a channel, freeing it for reuse. */
void snd_close(int ch);
