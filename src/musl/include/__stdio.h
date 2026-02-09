#ifndef __STDIO_H
#define __STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#define TYPEDEF typedef
#define STRUCT struct

TYPEDEF __builtin_va_list va_list;
TYPEDEF __builtin_va_list __isoc_va_list;
#define FILE void

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
int __fseeko(FILE *f, long long off, int whence);
int __fgetc(FILE *f);
int __ungetc(int c, FILE *f);
char* __fgets(char *restrict s, int n, FILE *restrict f);
long __ftell(FILE *);
long long __ftello(FILE *);
#ifndef fpos_t
typedef _fpos_t fpos_t;
#endif
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
char* __tmpnam(char *buf);
FILE* __tmpfile(void);
int __fileno(FILE*);

#undef FILE

#ifdef __cplusplus
}
#endif

#endif // __STDIO_H
