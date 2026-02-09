#include "internal/stdio_impl.h"
#include "internal/__stdio.h"
#include "unistd.h"
#include "sys_table.h"

static int __libc() dummy(int fd)
{
	return fd;
}

weak_alias(dummy, __aio_close);

int __libc() __stdio_close(FILE *f)
{
	return __close(__aio_close(f->fd));
}
