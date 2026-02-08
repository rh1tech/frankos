#ifndef PS2KBD_WRAPPER_H
#define PS2KBD_WRAPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ps2kbd_init(void);
void ps2kbd_tick(void);
int ps2kbd_get_key(int* pressed, unsigned char* key);
int ps2kbd_get_key_ext(int* pressed, unsigned char* key, uint8_t* hid_code);

#ifdef __cplusplus
}
#endif

#endif
