#ifndef _GETOPT_H
#define _GETOPT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

inline static int getopt(int argc, char * const argv[], const char *optstring) {
    typedef int (*fn_ptr_t)(int, char * const [], const char*);
    return ((fn_ptr_t)_sys_table_ptrs[344])(argc, argv, optstring);
}

#define optarg (*(char**)_sys_table_ptrs[345])
#define optind (*(int*)_sys_table_ptrs[346])
#define opterr (*(int*)_sys_table_ptrs[347])
#define optopt (*(int*)_sys_table_ptrs[348])
#define __optreset (*(int*)_sys_table_ptrs[349])

struct option {
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

///int getopt_long(int, char *const *, const char *, const struct option *, int *);
///int getopt_long_only(int, char *const *, const char *, const struct option *, int *);

#define no_argument        0
#define required_argument  1
#define optional_argument  2

#ifdef __cplusplus
}
#endif

#endif
