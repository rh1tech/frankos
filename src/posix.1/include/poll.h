#ifndef _POLL_H
#define _POLL_H

#include <sys/cdefs.h>   /* for __BEGIN_DECLS / __END_DECLS macros */
#include <sys/types.h>   /* for nfds_t */

__BEGIN_DECLS

/* Structure used by poll() */
struct pollfd {
    int   fd;        /* File descriptor to monitor */
    short events;    /* Events of interest */
    short revents;   /* Events that actually occurred (filled by the kernel) */
};

/* --- Event flags for pollfd.events and pollfd.revents --- */

/* Events to wait for */
#define POLLIN      0x0001  /* There is data to read */
#define POLLPRI     0x0002  /* Urgent (high-priority) data to read */
#define POLLOUT     0x0004  /* Writing is now possible */

/* Returned conditions / errors */
#define POLLERR     0x0008  /* Error condition on the descriptor */
#define POLLHUP     0x0010  /* Hangup or stream closed (EOF) */
#define POLLNVAL    0x0020  /* Invalid file descriptor */

/* Non-standard (but widely available) extensions */
#define POLLRDNORM  0x0040  /* Normal data may be read */
#define POLLRDBAND  0x0080  /* Priority data may be read */
#define POLLWRNORM  POLLOUT /* Normal data may be written */
#define POLLWRBAND  0x0100  /* Priority data may be written */

/* Type for number of file descriptors */
typedef unsigned long nfds_t;

/* Main function prototype */
int __poll(struct pollfd fds[], nfds_t nfds, int timeout);

__END_DECLS

#endif /* _POLL_H */
