#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "tusb.h"
#include "FreeRTOS.h"
#include "task.h"
#include "board_config.h"
#include "display.h"
#include "hdmi.h"
#include "hardware/structs/bus_ctrl.h"
#include "window.h"
#include "window_event.h"
#include "ps2.h"
#include "ps2kbd_wrapper.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

/* Shared dirty flag — set by input_task (Core 0), read by Core 1 */
static volatile bool g_video_dirty = true;

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

/*==========================================================================
 * Core 1 — bare-metal HDMI + compositor (no FreeRTOS)
 *
 * The entire video pipeline runs here: the DMA IRQ handler feeds
 * scanlines to the PIO, and between frames the main loop dispatches
 * window events and recomposites the scene.  Nothing on this core
 * touches FreeRTOS, so no BASEPRI masking or PendSV context switches
 * can ever delay the DMA IRQ or cause bus-stall cascades.
 *=========================================================================*/
static void core1_entry(void) {
    graphics_init_irq_on_this_core();

    for (;;) {
        /* Only recomposite when something changed */
        if (g_video_dirty) {
            g_video_dirty = false;

            /* Dispatch events that arrived from Core 0 */
            wm_dispatch_events();

            /* Redraw the scene into the draw buffer and swap */
            wm_composite();

            /* Wait until the HDMI vsync handler has applied the swap
             * so we never write into a buffer that DMA is still reading. */
            while (!graphics_swap_completed()) {
                tight_loop_contents();
            }
        } else {
            tight_loop_contents();
        }
    }
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

static void input_task(void *params) {
    (void)params;

    /* Absolute cursor position — start at screen center */
    int16_t cur_x = DISPLAY_WIDTH / 2;
    int16_t cur_y = DISPLAY_HEIGHT / 2;
    uint8_t prev_buttons = 0;

    for (;;) {
        /* Poll keyboard */
        ps2kbd_tick();

        int pressed;
        unsigned char key;
        uint8_t hid_code;
        while (ps2kbd_get_key_ext(&pressed, &key, &hid_code)) {
            printf("KEY %s: ascii=0x%02X hid=0x%02X\n",
                   pressed ? "DN" : "UP", key, hid_code);
        }

        /* Poll mouse */
        int16_t dx, dy;
        int8_t wheel;
        uint8_t buttons;
        if (ps2_mouse_get_state(&dx, &dy, &wheel, &buttons)) {
            /* Convert deltas to absolute — PS/2 Y is inverted */
            cur_x += dx;
            cur_y -= dy;

            /* Clamp to screen bounds */
            if (cur_x < 0) cur_x = 0;
            if (cur_x >= DISPLAY_WIDTH) cur_x = DISPLAY_WIDTH - 1;
            if (cur_y < 0) cur_y = 0;
            if (cur_y >= DISPLAY_HEIGHT) cur_y = DISPLAY_HEIGHT - 1;

            wm_set_cursor_pos(cur_x, cur_y);
            wm_set_mouse_buttons(buttons);

            /* Post mouse move event */
            window_event_t ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = WM_MOUSEMOVE;
            ev.mouse.x = cur_x;
            ev.mouse.y = cur_y;
            ev.mouse.buttons = buttons;
            wm_post_event_focused(&ev);

            /* Detect button transitions */
            uint8_t changed = buttons ^ prev_buttons;
            if (changed & 0x01) { /* left button */
                memset(&ev, 0, sizeof(ev));
                ev.type = (buttons & 0x01) ? WM_LBUTTONDOWN : WM_LBUTTONUP;
                ev.mouse.x = cur_x;
                ev.mouse.y = cur_y;
                ev.mouse.buttons = buttons;
                wm_post_event_focused(&ev);
            }
            if (changed & 0x02) { /* right button */
                memset(&ev, 0, sizeof(ev));
                ev.type = (buttons & 0x02) ? WM_RBUTTONDOWN : WM_RBUTTONUP;
                ev.mouse.x = cur_x;
                ev.mouse.y = cur_y;
                ev.mouse.buttons = buttons;
                wm_post_event_focused(&ev);
            }
            prev_buttons = buttons;

            /* Signal Core 1 to recomposite */
            g_video_dirty = true;
        }

        vTaskDelay(pdMS_TO_TICKS(8));
    }
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

    printf("Initializing PS/2 keyboard...\n"); stdio_flush();
    ps2kbd_init();
    printf("PS/2 keyboard initialized\n"); stdio_flush();

    printf("Initializing PS/2 mouse...\n"); stdio_flush();
    if (ps2_mouse_pio_init(pio0, PS2_MOUSE_CLK)) {
        if (ps2_mouse_init_device()) {
            printf("PS/2 mouse initialized (wheel=%d)\n", ps2_mouse_has_wheel());
        } else {
            printf("PS/2 mouse device init failed\n");
        }
    } else {
        printf("PS/2 mouse PIO init failed\n");
    }
    stdio_flush();

    wm_init();

    hwnd_t test_win = wm_create_window(100, 80, 300, 200,
                                        "Test Window", WSTYLE_DEFAULT,
                                        NULL, NULL);
    wm_set_focus(test_win);
    printf("Test window created (hwnd=%d)\n", test_win); stdio_flush();

    /* One-shot composite so the first frame is visible before Core 1 starts */
    wm_composite();

    /* Give DMA high bus priority so HDMI palette-conversion DMA
     * always wins SRAM arbitration over Core 0's FreeRTOS writes. */
    bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_DMA_R_BITS |
                            BUSCTRL_BUS_PRIORITY_DMA_W_BITS;

    multicore_launch_core1(core1_entry);
    sleep_ms(100);
    printf("Core 1 launched (bare-metal HDMI + compositor)\n"); stdio_flush();

    xTaskCreate(usb_service_task, "usb", 256, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(heartbeat_task, "heartbeat", 512, NULL, 1, NULL);
    xTaskCreate(input_task, "input", 512, NULL, 3, NULL);

    // xTaskCreate leaves BASEPRI raised (ulCriticalNesting = 0xaaaaaaaa by design).
    // Clear it so printf works before the scheduler starts.
    __asm volatile ("msr basepri, %0" :: "r"(0));

    printf("Starting scheduler...\n"); stdio_flush();

    vTaskStartScheduler();
    for (;;) { tight_loop_contents(); }
}
