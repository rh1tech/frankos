#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#include <sys/types.h>   /* for size_t, ssize_t, etc. */
#include <termios.h>     /* for termios structs if needed */
#include <linux/types.h> /* optional, for kernel types */
#include <bits/ioctls.h> /* optional internal constants */

/* Begin extern "C" if C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------
 * Main function prototype
 * -------------------------------------------------------------
 */

/**
 * ioctl - control device parameters
 * @fd: file descriptor
 * @request: operation code
 * @...: pointer to argument, depends on request
 *
 * Returns: 0 on success, -1 on error (errno set)
 */
int ioctl(int fd, unsigned long request, ...);

/* -------------------------------------------------------------
 * Terminal window size (TIOCGWINSZ / TIOCSWINSZ)
 * -------------------------------------------------------------
 */
struct winsize {
    unsigned short ws_row;    /* number of rows (height) */
    unsigned short ws_col;    /* number of columns (width) */
    unsigned short ws_xpixel; /* horizontal size in pixels (optional) */
    unsigned short ws_ypixel; /* vertical size in pixels (optional) */
};

/* Get window size */
#define TIOCGWINSZ 0x5413
/* Set window size */
#define TIOCSWINSZ 0x5414

/* -------------------------------------------------------------
 * Common terminal ioctl constants
 * -------------------------------------------------------------
 */
#define TIOCEXCL    0x540C  /* set exclusive use of tty */
#define TIOCNXCL    0x540D  /* reset exclusive use */
#define TIOCSTI     0x5412  /* simulate input */
#define TIOCMGET    0x5415  /* get modem status */
#define TIOCMBIS    0x5416  /* set bits in modem status */
#define TIOCMBIC    0x5417  /* clear bits in modem status */
#define TIOCMSET    0x5418  /* set modem status */
#define TIOCGPGRP   0x540F  /* get foreground process group */
#define TIOCSPGRP   0x5410  /* set foreground process group */
#define TIOCSWINSZ  0x5414  /* set window size (duplicate for clarity) */

/* -------------------------------------------------------------
 * Example macros for file descriptor control
 * -------------------------------------------------------------
 */
#define FIONREAD    0x541B  /* get number of bytes available for reading */
#define FIONBIO     0x5421  /* set/clear non-blocking mode */
#define FIOASYNC    0x5422  /* set/clear async mode */
#define FIOSETOWN   0x8901  /* set owner for SIGIO */
#define FIOGETOWN   0x8903  /* get owner for SIGIO */

/* End extern "C" if C++ */
#ifdef __cplusplus
}
#endif

#endif /* _SYS_IOCTL_H */
