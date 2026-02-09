#include "internal/stdio_impl.h"
#include "internal/__stdio.h"
#include "sys/ioctl.h"
#include "unistd.h"
#include "sys_table.h"

size_t __libc() __stdout_write(FILE *f, const unsigned char *buf, size_t len)
{
	struct winsize wsz;
	f->write = __stdio_write;
	if (!(f->flags & F_SVB) && __ioctl(f->fd, TIOCGWINSZ, &wsz))
		f->lbf = -1;
	return __stdio_write(f, buf, len);
}
