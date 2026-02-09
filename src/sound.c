#include <pico.h>
#include <hardware/pwm.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include "FreeRTOS.h"
#include "task.h"
#include "sound.h"
#include "audio.h"

#define SOUND_FREQUENCY 44100

#ifdef I2S_SOUND
static i2s_config_t i2s_config = {
		.sample_freq = SOUND_FREQUENCY, 
		.channel_count = 2,
		.data_pin = PWM_PIN0,
		.clock_pin_base = PWM_PIN1,
		.pio = pio1,
		.sm = 0,
        .dma_channel = 0,
        .dma_buf = NULL,
        .dma_trans_count = 0,
        .volume = 0,
	};
#else
static void PWM_init_pin(uint8_t pinN, uint16_t max_lvl) {
    pwm_config config = pwm_get_default_config();
    gpio_set_function(pinN, GPIO_FUNC_PWM);
    pwm_config_set_clkdiv(&config, 1.0);
    pwm_config_set_wrap(&config, max_lvl); // MAX PWM value
    pwm_init(pwm_gpio_to_slice_num(pinN), &config, true);
}
#endif

inline static void inInit(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}

void init_sound() {
#ifdef I2S_SOUND
    i2s_config.sample_freq = SOUND_FREQUENCY;
    i2s_config.dma_trans_count = SOUND_FREQUENCY / 100;
    i2s_volume(&i2s_config, 0);
#else
    PWM_init_pin(PWM_PIN0, (1 << 12) - 1);
    PWM_init_pin(PWM_PIN1, (1 << 12) - 1);
    PWM_init_pin(BEEPER_PIN, (1 << 12) - 1);
#endif
#ifdef WAV_IN_PIO
    //пин ввода звука
    inInit(WAV_IN_PIO);
#endif
}

void blimp(uint32_t count, uint32_t tiks_to_delay) {
#ifndef I2S_SOUND
    for (uint32_t i = 0; i < count; ++i) {
        vTaskDelay(tiks_to_delay);
        pwm_set_gpio_level(BEEPER_PIN, (1 << 12) - 1);
        vTaskDelay(tiks_to_delay);
        pwm_set_gpio_level(BEEPER_PIN, 0);
    }
#else
    // TODO:
#endif
}

static repeating_timer_t m_timer = { 0 };
static volatile pcm_end_callback_t m_cb = NULL;
#ifndef I2S_SOUND
static volatile int16_t* m_buff = NULL;
static volatile uint8_t m_channels = 0;
static volatile size_t m_off = 0; // in 16-bit words
static volatile size_t m_size = 0; // 16-bit values prepared (available)
static volatile bool m_let_process_it = false;
#endif

static bool __not_in_flash_func(timer_callback)(repeating_timer_t *rt) { // core#1?
#ifdef I2S_SOUND
    size_t size = 0;
    char* buff = m_cb(&size);
    i2s_config.dma_buf = buff;
    i2s_config.dma_trans_count = size >> 1;
    i2s_dma_write(&i2s_config, buff);
#else
    m_let_process_it = true;
#endif
    return true;
}

void pcm_call() {
#ifndef I2S_SOUND
    if (!m_let_process_it) {
        return;
    }
    static uint16_t outL = 0;
    static uint16_t outR = 0;
    pwm_set_gpio_level(PWM_PIN0, outR); // Право
    pwm_set_gpio_level(PWM_PIN1, outL); // Лево
    outL = outR = 0;

    if (!m_channels || !m_buff || m_off >= m_size) {
        m_let_process_it = false;
        return;
    }
    int16_t* b = m_buff + m_off;
    uint32_t x = ((int32_t)*b) + 0x8000;
    outL = x >> 4;
    ++m_off;
    if (m_channels == 2) {
        ++b;
        x = ((int32_t)*b) + 0x8000;
        outR = x >> 4;
        ++m_off;
    } else {
        outR = outL;
    }
    if (m_cb && m_off >= m_size) {
        m_buff = m_cb(&m_size);
        m_off = 0;
    }
    m_let_process_it = false;
#endif
    return;
}

void pcm_cleanup(void) {
    cancel_repeating_timer(&m_timer);
    m_timer.delay_us = 0;
#ifdef I2S_SOUND
    i2s_volume(&i2s_config, 0);
    i2s_deinit(&i2s_config);
    // TODO: stop DMA?
#else
    uint16_t o = 0;
    pwm_set_gpio_level(PWM_PIN0, o); // Право
    pwm_set_gpio_level(PWM_PIN1, o); // Лево
    m_let_process_it = false;
#endif
}

void pcm_setup(int hz) {
    if (m_timer.delay_us) {
        pcm_cleanup();
    }
#ifdef I2S_SOUND
    i2s_config.sample_freq = hz;
#else
    m_let_process_it = false;
    //hz; // 44100;	//44000 //44100 //96000 //22050
	// negative timeout means exact delay (rather than delay between callbacks)
	add_repeating_timer_us(-1000000 / hz, timer_callback, NULL, &m_timer);
#endif
}

// size - in 16-bit values count
void pcm_set_buffer(int16_t* buff, uint8_t channels, size_t size, pcm_end_callback_t cb) {
    m_cb = cb;
#ifdef I2S_SOUND
    i2s_config.channel_count = channels;
    i2s_config.dma_trans_count = i2s_config.sample_freq / (size << 1); // Number of 32 bits words to transfer
    i2s_config.dma_buf = buff;
    i2s_volume(&i2s_config, 16);
    i2s_init(&i2s_config);
    i2s_dma_write(&i2s_config, buff);
	add_repeating_timer_us(1000000 * size * channels / i2s_config.sample_freq, timer_callback, NULL, &m_timer);
#else
    m_size = 0;
    m_buff = buff;
    m_channels = channels;
    m_off = 0;
    m_size = size;
#endif
}
