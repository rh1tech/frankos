#include "internal/stdio_impl.h"
#include "internal/__stdio.h"
#include "errno.h"
#include "sys_table.h"
#include <string.h>

size_t __libc() __fwritex(const unsigned char *restrict s, size_t l, FILE *restrict f)
{
	size_t i=0;

	if (!f->wend && __towrite(f)) return 0;

	if (l > f->wend - f->wpos) return f->write(f, s, l);

	if (f->lbf >= 0) {
		/* Match /^(.*\n|)/ */
		for (i=l; i && s[i-1] != '\n'; i--);
		if (i) {
			size_t n = f->write(f, s, i);
			if (n < i) return n;
			s += i;
			l -= i;
		}
	}

	memcpy(f->wpos, s, l);
	f->wpos += l;
	return l+i;
}

size_t __libc() __fwrite(const void *restrict src, size_t size, size_t nmemb, FILE *restrict f)
{
	size_t k, l = size*nmemb;
	if (!size) nmemb = 0;
	FLOCK(f);
	k = __fwritex(src, l, f);
	FUNLOCK(f);
	return k==l ? nmemb : k/size;
}

weak_alias(__fwrite, fwrite_unlocked);

int __libc() __fputs(const char *restrict s, FILE *restrict f)
{
	size_t l = strlen(s);
	return (__fwrite(s, 1, l, f)==l) - 1;
}

weak_alias(__fputs, fputs_unlocked);

void __libc() __perror(const char *msg)
{
	FILE *f = stderr;
	char *errstr = strerror(errno);

	FLOCK(f);

	/* Save stderr's orientation and encoding rule, since perror is not
	 * permitted to change them. */
	void *old_locale = f->locale;
	int old_mode = f->mode;
	
	if (msg && *msg) {
		__fwrite(msg, strlen(msg), 1, f);
		__fputc(':', f);
		__fputc(' ', f);
	}
	__fwrite(errstr, strlen(errstr), 1, f);
	__fputc('\n', f);

	f->mode = old_mode;
	f->locale = old_locale;

	FUNLOCK(f);
}

/* The behavior of this function is undefined except when it is the first
 * operation on the stream, so the presence or absence of locking is not
 * observable in a program whose behavior is defined. Thus no locking is
 * performed here. No allocation of buffers is performed, but a buffer
 * provided by the caller is used as long as it is suitably sized. */

int __setvbuf(FILE *restrict f, char *restrict buf, int type, size_t size)
{
	f->lbf = EOF;

	if (type == _IONBF) {
		f->buf_size = 0;
	} else if (type == _IOLBF || type == _IOFBF) {
		if (buf && size >= UNGET) {
			f->buf = (void *)(buf + UNGET);
			f->buf_size = size - UNGET;
		}
		if (type == _IOLBF && f->buf_size)
			f->lbf = '\n';
	} else {
		return -1;
	}

	f->flags |= F_SVB;

	return 0;
}
