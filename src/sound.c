/*
 * Originally from Murmulator OS 2 by DnCraptor
 * https://github.com/DnCraptor/murmulator-os2
 *
 * Modified for FRANK OS — backward-compatible wrappers over snd_* mixer.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <pico.h>
#include <pico/stdlib.h>
#include "sound.h"
#include "snd.h"

/* Channel used by the legacy pcm_init / pcm_write API */
static int pcm_channel = -1;

void init_sound() {
    /* snd_init() is called separately from main.c */
}

void blimp(uint32_t count, uint32_t tiks_to_delay) {
    /* TODO: implement via tone generation on a snd channel */
    (void)count; (void)tiks_to_delay;
}

void pcm_call() {
    return;
}

void pcm_cleanup(void) {
    if (pcm_channel >= 0) {
        snd_close(pcm_channel);
        pcm_channel = -1;
    }
}

void pcm_setup(int hz) {
    (void)hz;
}

void pcm_set_buffer(int16_t* buff, uint8_t channels, size_t size, pcm_end_callback_t cb) {
    /* Timer-callback path removed — use pcm_init / pcm_write instead */
    (void)buff; (void)channels; (void)size; (void)cb;
}

void pcm_init(int sample_rate, int channels) {
    (void)channels;
    if (pcm_channel >= 0) snd_close(pcm_channel);
    pcm_channel = snd_open(sample_rate);
}

void pcm_write(const int16_t *samples, int count) {
    if (pcm_channel >= 0)
        snd_write(pcm_channel, samples, count);
}
