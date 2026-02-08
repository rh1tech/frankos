#include "window_event.h"
#include <string.h>

/*==========================================================================
 * Internal state
 *=========================================================================*/

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

/*==========================================================================
 * Event dispatch — stub implementations
 *=========================================================================*/

bool wm_post_event(hwnd_t hwnd, const window_event_t *event) {
    if (!event || eq_count >= WM_EVENT_QUEUE_SIZE) return false;

    event_queue[eq_head] = *event;
    event_target[eq_head] = hwnd;
    eq_head = (eq_head + 1) % WM_EVENT_QUEUE_SIZE;
    eq_count++;
    return true;
}

bool wm_post_event_focused(const window_event_t *event) {
    hwnd_t focus = wm_get_focus();
    if (focus == HWND_NULL) return false;
    return wm_post_event(focus, event);
}

void wm_dispatch_events(void) {
    while (eq_count > 0) {
        hwnd_t hwnd = event_target[eq_tail];
        window_event_t *ev = &event_queue[eq_tail];
        eq_tail = (eq_tail + 1) % WM_EVENT_QUEUE_SIZE;
        eq_count--;

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
    return mouse_pos;
}

bool wm_is_key_down(uint8_t scancode) {
    if (scancode >= 128) return false;
    return (key_state[scancode >> 3] & (1u << (scancode & 7))) != 0;
}

uint8_t wm_get_modifiers(void) {
    return modifier_state;
}
