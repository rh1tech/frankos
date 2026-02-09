#include "internal/stdio_impl.h"
#include "internal/__stdio.h"
#include "sys_table.h"
#include <errno.h>

int __libc() __fseeko_unlocked(FILE *f, off_t off, int whence)
{
	/* Fail immediately for invalid whence argument. */
	if (whence != SEEK_CUR && whence != SEEK_SET && whence != SEEK_END) {
		errno = EINVAL;
		return -1;
	}

	/* Adjust relative offset for unread data in buffer, if any. */
	if (whence == SEEK_CUR && f->rend) off -= f->rend - f->rpos;
	/* Flush write buffer, and report error on failure. */
	if (f->wpos != f->wbase) {
		f->write(f, 0, 0);
		if (!f->wpos) return -1;
	}

	/* Leave writing mode */
	f->wpos = f->wbase = f->wend = 0;

	/* Perform the underlying seek. */
	if (f->seek(f, off, whence) < 0) {
		return -1;
	}

	/* If seek succeeded, file is seekable and we discard read buffer. */
	f->rpos = f->rend = 0;
	f->flags &= ~F_EOF;
	
	return 0;
}

int __libc() __fseeko(FILE *f, off_t off, int whence)
{
	int result;
	FLOCK(f);
	result = __fseeko_unlocked(f, off, whence);
	FUNLOCK(f);
	return result;
}

int __libc() __fseek(FILE *f, long off, int whence)
{
	return __fseeko(f, off, whence);
}

weak_alias(__fseeko, fseeko);


void __libc() __rewind(FILE *f)
{
	FLOCK(f);
	__fseeko_unlocked(f, 0, SEEK_SET);
	f->flags &= ~F_ERR;
	FUNLOCK(f);
}

off_t __libc() __ftello_unlocked(FILE *f)
{
	off_t pos = f->seek(f, 0,
		(f->flags & F_APP) && f->wpos != f->wbase ? SEEK_END : SEEK_CUR
	);
	if (pos < 0) return pos;

	/* Adjust for data in buffer. */
	if (f->rend)
		pos += f->rpos - f->rend;
	else if (f->wbase)
		pos += f->wpos - f->wbase;
	return pos;
}

off_t __libc() __ftello(FILE *f)
{
	off_t pos;
	FLOCK(f);
	pos = __ftello_unlocked(f);
	FUNLOCK(f);
	return pos;
}

int __libc() __fgetpos(FILE *restrict f, fpos_t *restrict pos)
{
	off_t off = __ftello(f);
	if (off < 0) return -1;
	*(long long *)pos = off;
	return 0;
}

int __libc() __fsetpos(FILE *f, const fpos_t *pos)
{
	return __fseeko(f, *(const long long *)pos, SEEK_SET);
}

int __libc() __feof(FILE *f)
{
	FLOCK(f);
	int ret = !!(f->flags & F_EOF);
	FUNLOCK(f);
	return ret;
}

weak_alias(__feof, feof_unlocked);
weak_alias(__feof, _IO_feof_unlocked);

int __libc() __ferror(FILE *f)
{
	FLOCK(f);
	int ret = !!(f->flags & F_ERR);
	FUNLOCK(f);
	return ret;
}

weak_alias(__ferror, ferror_unlocked);
weak_alias(__ferror, _IO_ferror_unlocked);

void __clearerr(FILE *f)
{
	FLOCK(f);
	f->flags &= ~(F_EOF|F_ERR);
	FUNLOCK(f);
}

weak_alias(__clearerr, clearerr_unlocked);
