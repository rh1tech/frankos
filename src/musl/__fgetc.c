#include "internal/stdio_impl.h"
#include "internal/__stdio.h"
#include "sys_table.h"

int __libc() __uflow(FILE *f)
{
	unsigned char c;
	if (!__toread(f) && f->read(f, &c, 1)==1) return c;
	return EOF;
}

int __libc() (getc_unlocked)(FILE *f)
{
	return getc_unlocked(f);
}

weak_alias (getc_unlocked, fgetc_unlocked);
weak_alias (getc_unlocked, _IO_getc_unlocked);

#ifdef __GNUC__
__attribute__((__noinline__))
#endif
static int locking_getc(FILE *f)
{
	/// TODO:
///	if (a_cas(&f->lock, 0, MAYBE_WAITERS-1)) __lockfile(f);
	int c = getc_unlocked(f);
///	if (a_swap(&f->lock, 0) & MAYBE_WAITERS)
///		__wake(&f->lock, 1, 1);
	return c;
}

static inline int do_getc(FILE *f)
{
	/// TODO:
///	int l = f->lock;
///	if (l < 0 || l && (l & ~MAYBE_WAITERS) == __pthread_self()->tid)
		return getc_unlocked(f);
///	return locking_getc(f);
}

int __libc() __fgetc(FILE *f)
{
	return do_getc(f);
}

int __libc() __ungetc(int c, FILE *f)
{
	if (c == EOF) return c;

	FLOCK(f);

	if (!f->rpos) __toread(f);
	if (!f->rpos || f->rpos <= f->buf - UNGET) {
		FUNLOCK(f);
		return EOF;
	}

	*--f->rpos = c;
	f->flags &= ~F_EOF;

	FUNLOCK(f);
	return (unsigned char)c;
}
