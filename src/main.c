#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "tusb.h"
#include "FreeRTOS.h"
#include "task.h"
#include "board_config.h"
#include "display.h"
#include "hdmi.h"
#include "window.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

static inline void led_init(void) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
}

// Hard fault handler — double-blink pattern using raw SIO (no SDK calls).
// Named isr_hardfault to override the weak symbol in Pico SDK's vector table.
void __attribute__((used)) isr_hardfault(void) {
    *(volatile unsigned int *)0xd0000038 = (1u << 25);
    for (;;) {
        for (int b = 0; b < 2; b++) {
            *(volatile unsigned int *)0xd0000018 = (1u << 25);
            for (volatile int i = 0; i < 10000000; i++);
            *(volatile unsigned int *)0xd0000020 = (1u << 25);
            for (volatile int i = 0; i < 10000000; i++);
        }
        for (volatile int i = 0; i < 60000000; i++);
    }
}

static void core1_entry(void) {
    graphics_init_irq_on_this_core();
    while (1) { tight_loop_contents(); }
}

static void usb_service_task(void *params) {
    (void)params;
    for (;;) {
        tud_task();
        vTaskDelay(1);
    }
}

static void heartbeat_task(void *params) {
    (void)params;
    int count = 0;
    for (;;) {
        gpio_put(LED_PIN, 1);
        printf("tick %d (frames: %lu)\n", count++, (unsigned long)get_frame_count());
        stdio_flush();
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_put(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void display_task(void *params) {
    (void)params;
    /* Static frame — suspend to isolate HDMI signal issue */
    vTaskSuspend(NULL);
}

int main(void) {
    if (!set_sys_clock_khz(CPU_CLOCK_MHZ * 1000, false)) {
        set_sys_clock_khz(252 * 1000, true);
    }

    stdio_init_all();
    for (int i = 0; i < 8; i++) { sleep_ms(500); }

    led_init();

    printf("\n== Rhea OS ==\n");
    printf("CPU: %lu MHz\n", (unsigned long)(clock_get_hz(clk_sys) / 1000000));
    printf("Scheduler: FreeRTOS\n");
    stdio_flush();

    printf("Initializing display...\n"); stdio_flush();
    display_init();
    printf("Display initialized\n"); stdio_flush();

    wm_init();

    hwnd_t test_win = wm_create_window(100, 80, 300, 200,
                                        "Test Window", WSTYLE_DEFAULT,
                                        NULL, NULL);
    wm_set_focus(test_win);
    printf("Test window created (hwnd=%d)\n", test_win); stdio_flush();

    wm_composite();

    multicore_launch_core1(core1_entry);
    sleep_ms(100);
    printf("Core 1 launched\n"); stdio_flush();

    xTaskCreate(usb_service_task, "usb", 256, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(heartbeat_task, "heartbeat", 512, NULL, 1, NULL);
    xTaskCreate(display_task, "display", 1024, NULL, 2, NULL);

    // xTaskCreate leaves BASEPRI raised (ulCriticalNesting = 0xaaaaaaaa by design).
    // Clear it so printf works before the scheduler starts.
    __asm volatile ("msr basepri, %0" :: "r"(0));

    printf("Starting scheduler...\n"); stdio_flush();

    vTaskStartScheduler();
    for (;;) { tight_loop_contents(); }
}
