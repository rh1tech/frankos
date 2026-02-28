/*
 * Originally from Murmulator OS 2 by DnCraptor
 * https://github.com/DnCraptor/murmulator-os2
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <FreeRTOS.h>
#include <task.h>

#include <stdarg.h>

#include "sys/ioctl.h"
#include "errno.h"
#include "terminal.h"

int __ioctl(int fd, unsigned long request, void* opt) {
    if (request == TIOCGWINSZ && (fd == 1 || fd == 2)) {
        struct winsize* ws = (struct winsize*)opt;
        terminal_t *t = terminal_get_active();
        ws->ws_col = t ? terminal_get_cols(t) : TERM_COLS;
        ws->ws_row = t ? terminal_get_rows(t) : TERM_ROWS;
        ws->ws_xpixel = ws->ws_col * TERM_FONT_W;
        ws->ws_ypixel = ws->ws_row * TERM_FONT_H;
        errno = 0;
        return 0;
    }
    /// TODO:
    errno = EINVAL;
    return -1;
}
