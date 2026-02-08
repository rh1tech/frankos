// PS/2 Keyboard Wrapper for Test Application
// Based on murmdoom implementation
// SPDX-License-Identifier: GPL-2.0-or-later

#include "board_config.h"
#include "ps2kbd_wrapper.h"
#include "ps2kbd_mrmltr.h"
#include <queue>

struct KeyEvent {
    int pressed;
    unsigned char key;
    uint8_t hid_code;  // Raw HID code for display
};

static std::queue<KeyEvent> event_queue;

// HID to ASCII mapping
static unsigned char hid_to_ascii(uint8_t code, bool shift) {
    // Letters a-z (0x04-0x1D)
    if (code >= 0x04 && code <= 0x1D) {
        char c = 'a' + (code - 0x04);
        return shift ? (c - 32) : c;  // Uppercase if shift
    }
    
    // Numbers 1-9, 0 (0x1E-0x27)
    if (code >= 0x1E && code <= 0x27) {
        if (shift) {
            // Shifted number row symbols
            const char shifted[] = "!@#$%^&*()";
            return shifted[code - 0x1E];
        }
        if (code == 0x27) return '0';
        return '1' + (code - 0x1E);
    }
    
    // Special keys
    switch (code) {
        case 0x28: return '\n';     // Enter
        case 0x29: return 0x1B;     // Escape
        case 0x2A: return '\b';     // Backspace
        case 0x2B: return '\t';     // Tab
        case 0x2C: return ' ';      // Space
        case 0x2D: return shift ? '_' : '-';
        case 0x2E: return shift ? '+' : '=';
        case 0x2F: return shift ? '{' : '[';
        case 0x30: return shift ? '}' : ']';
        case 0x31: return shift ? '|' : '\\';
        case 0x33: return shift ? ':' : ';';
        case 0x34: return shift ? '"' : '\'';
        case 0x35: return shift ? '~' : '`';
        case 0x36: return shift ? '<' : ',';
        case 0x37: return shift ? '>' : '.';
        case 0x38: return shift ? '?' : '/';
        
        // Arrow keys - return special codes
        case 0x4F: return 0x80;  // Right arrow
        case 0x50: return 0x81;  // Left arrow
        case 0x51: return 0x82;  // Down arrow
        case 0x52: return 0x83;  // Up arrow
        
        // Function keys F1-F12
        case 0x3A: return 0xF1;  // F1
        case 0x3B: return 0xF2;  // F2
        case 0x3C: return 0xF3;  // F3
        case 0x3D: return 0xF4;  // F4
        case 0x3E: return 0xF5;  // F5
        case 0x3F: return 0xF6;  // F6
        case 0x40: return 0xF7;  // F7
        case 0x41: return 0xF8;  // F8
        case 0x42: return 0xF9;  // F9
        case 0x43: return 0xFA;  // F10
        case 0x44: return 0xFB;  // F11
        case 0x45: return 0xFC;  // F12
    }
    
    return 0;
}

static bool shift_held = false;

static void key_handler(hid_keyboard_report_t *curr, hid_keyboard_report_t *prev) {
    // Track shift state
    shift_held = (curr->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT)) != 0;
    
    // Check modifiers
    uint8_t changed_mods = curr->modifier ^ prev->modifier;
    if (changed_mods) {
        // Report modifier changes
        if (changed_mods & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT)) {
            int pressed = (curr->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT)) != 0;
            event_queue.push({pressed, 0xE1, 0xE1});  // Shift
        }
        if (changed_mods & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL)) {
            int pressed = (curr->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL)) != 0;
            event_queue.push({pressed, 0xE0, 0xE0});  // Ctrl
        }
        if (changed_mods & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT)) {
            int pressed = (curr->modifier & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT)) != 0;
            event_queue.push({pressed, 0xE2, 0xE2});  // Alt
        }
    }

    // Check for new key presses
    for (int i = 0; i < 6; i++) {
        if (curr->keycode[i] != 0) {
            bool found = false;
            for (int j = 0; j < 6; j++) {
                if (prev->keycode[j] == curr->keycode[i]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                unsigned char ascii = hid_to_ascii(curr->keycode[i], shift_held);
                event_queue.push({1, ascii, curr->keycode[i]});
            }
        }
    }

    // Check for key releases
    for (int i = 0; i < 6; i++) {
        if (prev->keycode[i] != 0) {
            bool found = false;
            for (int j = 0; j < 6; j++) {
                if (curr->keycode[j] == prev->keycode[i]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                unsigned char ascii = hid_to_ascii(prev->keycode[i], shift_held);
                event_queue.push({0, ascii, prev->keycode[i]});
            }
        }
    }
}

static Ps2Kbd_Mrmltr* kbd = nullptr;

extern "C" void ps2kbd_init(void) {
    // PS2 keyboard driver expects base_gpio as CLK, and base_gpio+1 as DATA
    kbd = new Ps2Kbd_Mrmltr(pio0, PS2_PIN_CLK, key_handler);
    kbd->init_gpio();
}

extern "C" void ps2kbd_tick(void) {
    if (kbd) kbd->tick();
}

extern "C" int ps2kbd_get_key(int* pressed, unsigned char* key) {
    if (event_queue.empty()) return 0;
    KeyEvent e = event_queue.front();
    event_queue.pop();
    *pressed = e.pressed;
    *key = e.key;
    return 1;
}

extern "C" int ps2kbd_get_key_ext(int* pressed, unsigned char* key, uint8_t* hid_code) {
    if (event_queue.empty()) return 0;
    KeyEvent e = event_queue.front();
    event_queue.pop();
    *pressed = e.pressed;
    *key = e.key;
    *hid_code = e.hid_code;
    return 1;
}
