#ifndef _GETOPT_H
#define _GETOPT_H

#ifdef __cplusplus
extern "C" {
#endif

int __getopt(int, char * const [], const char *);
extern char *__optarg;
extern int __optind, __opterr, __optopt, __optreset;

struct option {
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

int __getopt_long(int, char *const *, const char *, const struct option *, int *);
int __getopt_long_only(int, char *const *, const char *, const struct option *, int *);

#define no_argument        0
#define required_argument  1
#define optional_argument  2

#ifdef __cplusplus
}
#endif

#endif
