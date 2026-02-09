#include "internal/pthread_impl.h"
#include <time.h>
#include <stdint.h>
#include "internal/__stdio.h"
#include "unistd.h"
#include "sys/fcntl.h"
#include "ff.h"
#include "sys_table.h"

#include <pico/time.h>

int __libc() __clock_gettime(clockid_t clk, struct timespec *ts32)
{
	/** TODO:
	struct timespec ts;
	int r = clock_gettime(clk, &ts);
	if (r) return r;
	if (ts.tv_sec < INT32_MIN || ts.tv_sec > INT32_MAX) {
		errno = EOVERFLOW;
		return -1;
	}
	ts32->tv_sec = ts.tv_sec;
	ts32->tv_nsec = ts.tv_nsec;
	*/
	uint64_t t = time_us_64();
	ts32->tv_sec = t / 1000000;
	ts32->tv_nsec = (t % 1000000) * 1000;
	if (CLOCK_REALTIME == clk) {
		ts32->tv_sec += 1761598873; // W/A Mon, 27 Oct 2025 21:01:13 GMT
	}
	return 0;
}

/* This assumes that a check for the
   template size has already been made */
char* __libc() __randname(char *template)
{
	int i;
	struct timespec ts;
	unsigned long r;

	__clock_gettime(CLOCK_REALTIME, &ts);
	r = ts.tv_sec + ts.tv_nsec + __pthread_tid() * 65537UL;
	for (i=0; i<6; i++, r>>=5)
		template[i] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"[r % 62];

	return template;
}

#define MAXTRIES 100

char* __libc() __tmpnam(char* buf) {
	static char internal[L_tmpnam];
	char s[] = "/tmp/tmpnam_XXXXXX";
	int try;
	FRESULT r;
    FILINFO* pf = (FILINFO*)pvPortMalloc(sizeof(FILINFO));
	if (!pf) return 0;
	for (try = 0; try < MAXTRIES; ++try) {
		__randname(s + 12);
		r = f_stat(s, pf);
		if (r != FR_OK) {
			vPortFree(pf);
			return buf ? strcpy(buf, s) : strcpy(internal, s);
		}
	}
	vPortFree(pf);
	return 0;
}

FILE* __libc() __tmpfile(void)
{
	char s[] = "/tmp/tmpfile_XXXXXX";
	int fd;
	FILE *f;
	int try;
	for (try=0; try<MAXTRIES; try++) {
		__randname(s+13);
		fd = __openat(AT_FDCWD, s, O_RDWR|O_CREAT|O_EXCL, 0600);
		if (fd >= 0) {
			__unlinkat(AT_FDCWD, s, 0);
			f = __fdopen(fd, "w+");
			if (!f) __close(fd);
			return f;
		}
	}
	return 0;
}
