#include <FreeRTOS.h>
#include <task.h>

#include <stdarg.h>

#include "sys/ioctl.h"
#include "errno.h"

int __ioctl(int fd, unsigned long request, void* opt) {
    if (request == TIOCGWINSZ && (fd == 1 || fd == 2)) {
        struct winsize* ws = (struct winsize*)opt;
        /// TODO: graphics_driver_t??
        ws->ws_col = 80;
        ws->ws_row = 30;
        ws->ws_xpixel = 640;
        ws->ws_ypixel = 480;
        errno = 0;
        return 0;
    }
    /// TODO:
    errno = EINVAL;
    return -1;
}
