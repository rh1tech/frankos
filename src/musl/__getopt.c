#define _OWN_IO_FILE 
#define _BSD_SOURCE
//#include <unistd.h>
//#include <wchar.h>
//#include <string.h>
//#include <stdlib.h>
//#include "locale_impl.h"
#include "internal/stdio_impl.h"
#include "internal/__stdio.h"
#include "internal/__getopt.h"
#include <limits.h>
#include "sys_table.h"

char *__optarg;
int __optind=1, __opterr=1, __optopt, __optpos, __optreset=0;

#define optpos __optpos
weak_alias(__optreset, optreset);

void __libc() __getopt_msg(const char *a, const char *b, const char *c, size_t l)
{
	FILE *f = stderr;
/// TODO:	b = __lctrans_cur(b);
	FLOCK(f);
	fputs(a, f)>=0
	&& fwrite(b, strlen(b), 1, f)
	&& fwrite(c, 1, l, f)==l
	&& putc('\n', f);
	FUNLOCK(f);
}

int __libc() __getopt(int argc, char * const argv[], const char *optstring)
{
	int i;
	wchar_t c, d;
	int k, l;
	char *optchar;

	if (!__optind || __optreset) {
		__optreset = 0;
		__optpos = 0;
		__optind = 1;
	}

	if (__optind >= argc || !argv[__optind])
		return -1;

	if (argv[__optind][0] != '-') {
		if (optstring[0] == '-') {
			__optarg = argv[__optind++];
			return 1;
		}
		return -1;
	}

	if (!argv[__optind][1])
		return -1;

	if (argv[__optind][1] == '-' && !argv[__optind][2])
		return __optind++, -1;

	if (!optpos) optpos++;
	if ((k = mbtowc(&c, argv[__optind]+optpos, MB_LEN_MAX)) < 0) {
		k = 1;
		c = 0xfffd; /* replacement char */
	}
	optchar = argv[__optind]+optpos;
	optpos += k;

	if (!argv[__optind][optpos]) {
		__optind++;
		optpos = 0;
	}

	if (optstring[0] == '-' || optstring[0] == '+')
		optstring++;

	i = 0;
	d = 0;
	do {
		l = mbtowc(&d, optstring+i, MB_LEN_MAX);
		if (l>0) i+=l; else i++;
	} while (l && d != c);

	if (d != c || c == ':') {
		__optopt = c;
		if (optstring[0] != ':' && __opterr)
			__getopt_msg(argv[0], ": unrecognized option: ", optchar, k);
		return '?';
	}
	if (optstring[i] == ':') {
		__optarg = 0;
		if (optstring[i+1] != ':' || optpos) {
			__optarg = argv[__optind++];
			if (optpos) __optarg += optpos;
			optpos = 0;
		}
		if (__optind > argc) {
			__optopt = c;
			if (optstring[0] == ':') return ':';
			if (__opterr) __getopt_msg(argv[0],
				": option requires an argument: ",
				optchar, k);
			return '?';
		}
	}
	return c;
}

weak_alias(__getopt, __posix_getopt);
