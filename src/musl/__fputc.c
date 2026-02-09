#include "internal/stdio_impl.h"
#include "internal/__stdio.h"
#include "sys_table.h"

int __libc() (putc_unlocked)(int c, FILE *f)
{
	return putc_unlocked(c, f);
}

weak_alias(putc_unlocked, fputc_unlocked);
weak_alias(putc_unlocked, _IO_putc_unlocked);

#ifdef __GNUC__
__attribute__((__noinline__))
#endif
static int locking_putc(int c, FILE *f)
{
	/// TODO:
///	if (a_cas(&f->lock, 0, MAYBE_WAITERS-1)) __lockfile(f);
	c = putc_unlocked(c, f);
///	if (a_swap(&f->lock, 0) & MAYBE_WAITERS)
///		__wake(&f->lock, 1, 1);
	return c;
}

static inline int do_putc(int c, FILE *f)
{
	/// TODO:
///	int l = f->lock;
///	if (l < 0 || l && (l & ~MAYBE_WAITERS) == __pthread_self()->tid)
		return putc_unlocked(c, f);
///	return locking_putc(c, f);
}

int __libc() __fputc(int c, FILE *f)
{
	return do_putc(c, f);
}

weak_alias(__fputc, _IO_putc);

int __libc() __overflow(FILE *f, int _c)
{
	unsigned char c = _c;
	if (!f->wend && __towrite(f)) return EOF;
	if (f->wpos != f->wend && c != f->lbf) return *f->wpos++ = c;
	if (f->write(f, &c, 1)!=1) return EOF;
	return c;
}
