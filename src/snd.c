/*
 * FRANK OS — Sound Mixer
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * Multi-channel ring-buffer mixer with nearest-neighbour resampling.
 * The DMA IRQ handler reads all active channels, resamples to 44100 Hz,
 * mixes (sum + clamp), and fills the DMA ping-pong buffer.  Playback is
 * completely decoupled from task scheduling.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "snd.h"
#include "audio.h"
#include "board_config.h"

#include <string.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/sync.h"
#include "FreeRTOS.h"
#include "task.h"

/* DMA chunk size in stereo frames — IRQ fires every ~23 ms at 44100 Hz */
#define SND_DMA_FRAMES   1024

/* Per-channel ring buffer size (must be power of 2) */
#define SND_CHAN_FRAMES   2048
#define SND_CHAN_MASK     (SND_CHAN_FRAMES - 1)

typedef struct {
    int16_t  buf[SND_CHAN_FRAMES * 2];  /* stereo L/R interleaved, 8 KB */
    volatile uint32_t rd;               /* read position  (IRQ increments) */
    volatile uint32_t wr;               /* write position (task increments) */
    uint32_t rate;                      /* source sample rate */
    uint32_t phase;                     /* fixed-point resampling accumulator */
    uint32_t phase_inc;                 /* (source_rate << 16) / 44100 */
    int16_t  hold_l;                    /* last output sample (sample-and-hold) */
    int16_t  hold_r;
    bool     active;
} snd_channel_t;

static snd_channel_t channels[SND_MAX_CHANNELS];

/* Global volume as right-shift (0 = max, higher = quieter) */
static uint8_t snd_volume = 0;

static i2s_config_t snd_i2s_config;

/*==========================================================================
 * snd_fill_dma — called from DMA IRQ to mix all channels into one buffer
 *
 * Runs in IRQ context — must be fast and must not block.
 * Placed in RAM so it works even during flash operations.
 *==========================================================================*/
static void __not_in_flash_func(snd_fill_dma)(int buf_index,
                                               uint32_t *buf,
                                               uint32_t frames) {
    (void)buf_index;
    int16_t *out = (int16_t *)buf;

    /* Snapshot available frames per channel (wr only changes from task
     * context, rd only from here — both are stable for this fill). */
    uint32_t ch_avail[SND_MAX_CHANNELS];
    for (int ch = 0; ch < SND_MAX_CHANNELS; ch++) {
        if (channels[ch].active)
            ch_avail[ch] = channels[ch].wr - channels[ch].rd;
        else
            ch_avail[ch] = 0;
    }

    for (uint32_t i = 0; i < frames; i++) {
        int32_t left = 0, right = 0;

        for (int ch = 0; ch < SND_MAX_CHANNELS; ch++) {
            snd_channel_t *c = &channels[ch];
            if (!c->active) continue;

            if (ch_avail[ch] == 0) {
                /* Buffer empty — hold last sample to avoid DC-offset click */
                left  += c->hold_l;
                right += c->hold_r;
                continue;
            }

            uint32_t src_idx = c->phase >> 16;
            if (src_idx >= ch_avail[ch]) {
                /* Exhausted available data — hold last sample */
                left  += c->hold_l;
                right += c->hold_r;
                continue;
            }

            uint32_t idx0 = (c->rd + src_idx) & SND_CHAN_MASK;
            int16_t l0 = c->buf[idx0 * 2];
            int16_t r0 = c->buf[idx0 * 2 + 1];

            int16_t sl, sr;
            if (src_idx + 1 < ch_avail[ch]) {
                /* Linear interpolation between current and next sample */
                uint32_t idx1 = (c->rd + src_idx + 1) & SND_CHAN_MASK;
                int32_t frac = (int32_t)((c->phase & 0xFFFF) >> 1);
                sl = l0 + (((int32_t)(c->buf[idx1 * 2]     - l0) * frac) >> 15);
                sr = r0 + (((int32_t)(c->buf[idx1 * 2 + 1] - r0) * frac) >> 15);
            } else {
                sl = l0;
                sr = r0;
            }

            c->hold_l = sl;
            c->hold_r = sr;
            left  += sl;
            right += sr;

            c->phase += c->phase_inc;
        }

        /* Volume: right-shift */
        if (snd_volume) {
            left  >>= snd_volume;
            right >>= snd_volume;
        }

        /* Clamp to int16_t range */
        if (left  >  32767) left  =  32767;
        if (left  < -32768) left  = -32768;
        if (right >  32767) right =  32767;
        if (right < -32768) right = -32768;

        out[i * 2]     = (int16_t)left;
        out[i * 2 + 1] = (int16_t)right;
    }

    /* Advance read pointers by the number of source frames consumed */
    for (int ch = 0; ch < SND_MAX_CHANNELS; ch++) {
        snd_channel_t *c = &channels[ch];
        if (ch_avail[ch] == 0) continue;

        uint32_t consumed = c->phase >> 16;
        c->rd    += consumed;
        c->phase &= 0xFFFF;  /* keep fractional part */
    }
}

/*==========================================================================
 * snd_init — start I2S at 44100 Hz, DMA plays silence until channels open
 *==========================================================================*/
void snd_init(void) {
    memset(channels, 0, sizeof(channels));

    memset(&snd_i2s_config, 0, sizeof(snd_i2s_config));
    snd_i2s_config.sample_freq    = SND_SYSTEM_RATE;
    snd_i2s_config.channel_count  = 2;
    snd_i2s_config.data_pin       = I2S_DATA_PIN;
    snd_i2s_config.clock_pin_base = I2S_CLOCK_PIN_BASE;
    snd_i2s_config.pio            = pio1;
    snd_i2s_config.dma_trans_count = SND_DMA_FRAMES;
    snd_i2s_config.volume         = 0;

    i2s_init(&snd_i2s_config);
    i2s_set_fill_callback(snd_fill_dma);
    i2s_start();
}

/*==========================================================================
 * snd_open — allocate a mixing channel
 *==========================================================================*/
int snd_open(uint32_t sample_rate) {
    for (int ch = 0; ch < SND_MAX_CHANNELS; ch++) {
        if (!channels[ch].active) {
            snd_channel_t *c = &channels[ch];
            memset(c->buf, 0, sizeof(c->buf));
            c->rd        = 0;
            c->wr        = 0;
            c->rate      = sample_rate;
            c->phase     = 0;
            c->phase_inc = (sample_rate << 16) / SND_SYSTEM_RATE;
            c->hold_l    = 0;
            c->hold_r    = 0;
            c->active    = true;
            return ch;
        }
    }
    return -1;  /* all channels in use */
}

/*==========================================================================
 * snd_write — copy stereo frames into a channel's ring buffer
 *
 * Blocks (vTaskDelay) if the ring buffer is full.  Handles wrap-around
 * with up to two memcpy calls per iteration.
 *==========================================================================*/
void snd_write(int ch, const int16_t *samples, int frames) {
    if (ch < 0 || ch >= SND_MAX_CHANNELS) return;
    snd_channel_t *c = &channels[ch];

    while (frames > 0) {
        uint32_t used  = c->wr - c->rd;
        uint32_t space = SND_CHAN_FRAMES - used;

        if (space == 0) {
            vTaskDelay(1);
            continue;
        }

        uint32_t n = (uint32_t)frames;
        if (n > space) n = space;

        uint32_t wr_idx = c->wr & SND_CHAN_MASK;
        uint32_t first  = SND_CHAN_FRAMES - wr_idx;
        if (first > n) first = n;

        /* Each stereo frame = 2 x int16_t = 4 bytes */
        memcpy(&c->buf[wr_idx * 2], samples, first * 4);
        if (n > first) {
            memcpy(&c->buf[0], samples + first * 2, (n - first) * 4);
        }

        __dmb();     /* ensure sample data visible before wr update */
        c->wr += n;

        samples += n * 2;
        frames  -= (int)n;
    }
}

/*==========================================================================
 * snd_close — release a mixing channel
 *==========================================================================*/
void snd_close(int ch) {
    if (ch < 0 || ch >= SND_MAX_CHANNELS) return;
    channels[ch].active = false;
    channels[ch].rd     = 0;
    channels[ch].wr     = 0;
    channels[ch].phase  = 0;
}
