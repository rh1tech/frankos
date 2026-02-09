#include "internal/stdio_impl.h"
#include "internal/__stdio.h"
#include "sys_table.h"

int __libc() __toread(FILE *f)
{
	f->mode |= f->mode-1;
	if (f->wpos != f->wbase) f->write(f, 0, 0);
	f->wpos = f->wbase = f->wend = 0;
	if (f->flags & F_NORD) {
		f->flags |= F_ERR;
		return EOF;
	}
	f->rpos = f->rend = f->buf + f->buf_size;
	return (f->flags & F_EOF) ? EOF : 0;
}

hidden __libc() void __toread_needs_stdio_exit()
{
	__stdio_exit_needed();
}
