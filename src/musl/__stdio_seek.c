#include "internal/stdio_impl.h"
#include "internal/__stdio.h"
#include "sys_table.h"

off_t __libc() __stdio_seek(FILE *f, off_t off, int whence)
{
	return __lseek(f->fd, off, whence);
}
