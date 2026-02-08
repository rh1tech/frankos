#include "window_event.h"
#include "display.h"
#include <string.h>
#include "hardware/sync.h"

/*==========================================================================
 * Internal state
 *=========================================================================*/

/* Hardware spinlock for the event queue — safe across both cores,
 * takes ~2 cycles, and does NOT raise BASEPRI (unlike FreeRTOS
 * critical sections) so it can never mask or delay the HDMI DMA IRQ. */
static spin_lock_t *eq_spinlock;

/* Circular event queue */
static window_event_t event_queue[WM_EVENT_QUEUE_SIZE];
static hwnd_t         event_target[WM_EVENT_QUEUE_SIZE]; /* target window */
static uint8_t        eq_head;
static uint8_t        eq_tail;
static uint8_t        eq_count;

/* Key state bitmap — 128 bits for scancodes 0x00-0x7F */
static uint8_t key_state[16];

/* Mouse state */
static point_t  mouse_pos;
static uint8_t  modifier_state;

/* Atomic cursor position: upper 16 bits = x, lower 16 bits = y.
 * Packed into one word for atomic load/store on Cortex-M33.
 * Initialized to screen center. */
static volatile uint32_t cursor_pos_packed =
    ((uint32_t)(uint16_t)(DISPLAY_WIDTH / 2) << 16) |
     (uint32_t)(uint16_t)(DISPLAY_HEIGHT / 2);

/* Live mouse button state (written by input_task, read by compositor) */
static volatile uint8_t mouse_buttons_live;

/* Compositor dirty flag — avoids recompositing idle frames.
 * Starts dirty so the first frame is always drawn. */
static volatile uint8_t compositor_dirty = 1;

void wm_event_init(void) {
    int lock_num = spin_lock_claim_unused(true);
    eq_spinlock = spin_lock_init(lock_num);
    eq_head = 0;
    eq_tail = 0;
    eq_count = 0;
}

/*==========================================================================
 * Event dispatch — stub implementations
 *=========================================================================*/

bool wm_post_event(hwnd_t hwnd, const window_event_t *event) {
    if (!event) return false;

    uint32_t save = spin_lock_blocking(eq_spinlock);
    if (eq_count >= WM_EVENT_QUEUE_SIZE) {
        spin_unlock(eq_spinlock, save);
        return false;
    }
    event_queue[eq_head] = *event;
    event_target[eq_head] = hwnd;
    eq_head = (eq_head + 1) % WM_EVENT_QUEUE_SIZE;
    eq_count++;
    spin_unlock(eq_spinlock, save);
    compositor_dirty = 1;
    return true;
}

bool wm_post_event_focused(const window_event_t *event) {
    hwnd_t focus = wm_get_focus();
    if (focus == HWND_NULL) return false;
    return wm_post_event(focus, event);
}

void wm_dispatch_events(void) {
    while (eq_count > 0) {
        uint32_t save = spin_lock_blocking(eq_spinlock);
        if (eq_count == 0) {
            spin_unlock(eq_spinlock, save);
            break;
        }
        hwnd_t hwnd = event_target[eq_tail];
        window_event_t ev_copy = event_queue[eq_tail];
        eq_tail = (eq_tail + 1) % WM_EVENT_QUEUE_SIZE;
        eq_count--;
        spin_unlock(eq_spinlock, save);

        window_event_t *ev = &ev_copy;

        /* Update internal key state tracking */
        if (ev->type == WM_KEYDOWN) {
            uint8_t sc = ev->key.scancode;
            key_state[sc >> 3] |= (1u << (sc & 7));
            modifier_state = ev->key.modifiers;
        } else if (ev->type == WM_KEYUP) {
            uint8_t sc = ev->key.scancode;
            key_state[sc >> 3] &= ~(1u << (sc & 7));
            modifier_state = ev->key.modifiers;
        } else if (ev->type == WM_MOUSEMOVE) {
            mouse_pos.x = ev->mouse.x;
            mouse_pos.y = ev->mouse.y;
        }

        /* Deliver to window's event handler */
        window_t *win = wm_get_window(hwnd);
        if (win && win->event_handler) {
            win->event_handler(hwnd, ev);
        }
    }
}

/*==========================================================================
 * Input state queries
 *=========================================================================*/

point_t wm_get_mouse_pos(void) {
    uint32_t packed = cursor_pos_packed;
    point_t p;
    p.x = (int16_t)(packed >> 16);
    p.y = (int16_t)(packed & 0xFFFF);
    return p;
}

bool wm_is_key_down(uint8_t scancode) {
    if (scancode >= 128) return false;
    return (key_state[scancode >> 3] & (1u << (scancode & 7))) != 0;
}

uint8_t wm_get_modifiers(void) {
    return modifier_state;
}

/*==========================================================================
 * Cursor position (atomic packed word)
 *=========================================================================*/

void wm_set_cursor_pos(int16_t x, int16_t y) {
    cursor_pos_packed = ((uint32_t)(uint16_t)x << 16) | (uint32_t)(uint16_t)y;
    compositor_dirty = 1;
}

void wm_get_cursor_pos(int16_t *x, int16_t *y) {
    uint32_t packed = cursor_pos_packed;
    *x = (int16_t)(packed >> 16);
    *y = (int16_t)(packed & 0xFFFF);
}

void wm_set_mouse_buttons(uint8_t buttons) {
    mouse_buttons_live = buttons;
}

uint8_t wm_get_mouse_buttons(void) {
    return mouse_buttons_live;
}

void wm_mark_dirty(void) {
    compositor_dirty = 1;
}

bool wm_needs_composite(void) {
    if (!compositor_dirty) return false;
    compositor_dirty = 0;
    return true;
}
