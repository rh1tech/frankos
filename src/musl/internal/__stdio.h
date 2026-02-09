#ifndef __STDIO_INTERNAL_H
#define __STDIO_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#undef off_t
#ifndef _OFF_T_DECLARED
typedef long long off_t;
typedef long long _off_t;
#define __machine_off_t_defined
#define	_OFF_T_DECLARED
#endif

#define weak_alias(old, new) \
	extern __typeof(old) new __attribute__((__weak__, __alias__(#old)))

#define TYPEDEF typedef
#define STRUCT struct

TYPEDEF __builtin_va_list va_list;
TYPEDEF __builtin_va_list __isoc_va_list;
#ifndef _OWN_IO_FILE
STRUCT _IO_FILE { char __x; };
TYPEDEF struct _IO_FILE FILE;
#endif

#ifndef NULL
#if __cplusplus >= 201103L
#define NULL nullptr
#elif defined(__cplusplus)
#define NULL 0L
#else
#define NULL ((void*)0)
#endif
#endif

#undef EOF
#define EOF (-1)

#undef SEEK_SET
#undef SEEK_CUR
#undef SEEK_END
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define BUFSIZ 1024
#define FILENAME_MAX 4096
#define FOPEN_MAX 1000
#define TMP_MAX 10000
#define L_tmpnam 20

#ifndef AT_FDCWD
#define AT_FDCWD (-100)
#endif

#define O_CREAT        0100
#define O_EXCL         0200
#define O_NOCTTY       0400
#define O_TRUNC       01000
#define O_APPEND      02000
#define O_NONBLOCK    04000
#define O_DSYNC      010000
#define O_SYNC     04010000
#define O_RSYNC    04010000
#define O_NOFOLLOW  0100000

///#define O_DIRECTORY  040000
///#define O_CLOEXEC  02000000

/* defined by POSIX Issue 7 */
#define	O_CLOEXEC	0x10000		/* atomically set FD_CLOEXEC */
#define	O_DIRECTORY	0x20000		/* fail if not a directory */

/* defined by POSIX Issue 8 */
#define	O_CLOFORK	0x40000		/* atomically set FD_CLOFORK */

#define O_ASYNC      020000
#define O_DIRECT    0200000
#define O_LARGEFILE 0400000
#define O_NOATIME  01000000
#define O_PATH    010000000
#define O_TMPFILE 020040000
#define O_NDELAY O_NONBLOCK

#define F_DUPFD  0
#define F_GETFD  1
#define F_SETFD  2
#define F_GETFL  3
#define F_SETFL  4

#define F_SETOWN 8
#define F_GETOWN 9
#define F_SETSIG 10
#define F_GETSIG 11

#define F_GETLK 12
#define F_SETLK 13
#define F_SETLKW 14

#define F_SETOWN_EX 15
#define F_GETOWN_EX 16

#define F_GETOWNER_UIDS 17

typedef union _G_fpos64_t {
	char __opaque[16];
	long long __lldata;
	double __align;
} fpos_t;

extern FILE *const stdin;
extern FILE *const stdout;
extern FILE *const stderr;

#define stdin  (stdin)
#define stdout (stdout)
#define stderr (stderr)

int __vfprintf(FILE *restrict f, const char *restrict fmt, va_list ap);
size_t __fwritex(const unsigned char *restrict s, size_t l, FILE *restrict f);
size_t __fwrite(const void *restrict src, size_t size, size_t nmemb, FILE *restrict f);
FILE* __fopen(const char *restrict filename, const char *restrict mode);
int __fclose(FILE *f);
int __fflush(FILE *f);
size_t __fread(void *__restrict b, size_t n1, size_t n2, FILE *__restrict f);
int __fputc(int c, FILE *f);
void __rewind(FILE *f);
int __fseek(FILE *f, long off, int whence);
int __fseeko(FILE *f, off_t off, int whence);
int __fgetc(FILE *f);
int __ungetc(int c, FILE *f);
char* __fgets(char *restrict s, int n, FILE *restrict f);
off_t __ftello(FILE *);
int __fgetpos(FILE *__restrict, fpos_t *__restrict);
int __fsetpos(FILE *, const fpos_t *);
int __feof(FILE *);
int __ferror(FILE *);
void __clearerr(FILE *);
FILE* __freopen(const char *__restrict, const char *__restrict, FILE *__restrict);
FILE *const __stdin();
FILE *const __stdout();
FILE *const __stderr();
int __fputs(const char *__restrict s, FILE *__restrict f);
int __vfscanf(FILE *restrict f, const char *restrict fmt, va_list ap);
int __vsnprintf(char *restrict s, size_t n, const char *restrict fmt, va_list ap);
int __vsscanf(const char *restrict s, const char *restrict fmt, va_list ap);
void __perror(const char *msg);
int __setvbuf(FILE *__restrict, char *__restrict, int, size_t);
char* __realpath(const char *restrict filename, char *restrict resolved);
char* __realpathat(int dfd, const char *restrict filename, char *restrict resolved, int flags);
char* __tmpnam(char *buf);
FILE* __tmpfile(void);
int __fileno(FILE*);

#ifdef __cplusplus
}
#endif

#endif // __STDIO_INTERNAL_H
