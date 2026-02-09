#ifndef	_STRING_H
#define	_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

#undef NULL
#if __cplusplus >= 201103L
#define NULL nullptr
#elif defined(__cplusplus)
#define NULL 0L
#else
#define NULL ((void*)0)
#endif

#define __NEED_size_t
#if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE) \
 || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) \
 || defined(_BSD_SOURCE)
#define __NEED_locale_t
#endif

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

#include <stddef.h>

inline static
int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *p = (const unsigned char *)a;
    const unsigned char *q = (const unsigned char *)b;

    while (n--) {
        unsigned char x = *p++;
        unsigned char y = *q++;
        if (x != y)
            return (x < y) ? -1 : 1;
    }
    return 0;
}

/*
void *memchr (const void *, int, size_t);

int strcoll (const char *, const char *);
size_t strxfrm (char *__restrict, const char *__restrict, size_t);

size_t strcspn (const char *, const char *);
size_t strspn (const char *, const char *);
char *strpbrk (const char *, const char *);
char *strtok (char *__restrict, const char *__restrict);
*/

inline static
char *strstr(const char *haystack, const char *needle)
{
    if (!*needle)
        return (char *)haystack;

    const unsigned char *h = (const unsigned char *)haystack;
    const unsigned char *n = (const unsigned char *)needle;
    unsigned char first = n[0];

    for (;; ++h) {
        unsigned char c = *h;
        if (c == 0)
            return 0;
        if (c != first)
            continue;

        /* candidate match */
        const unsigned char *h2 = h + 1;
        const unsigned char *n2 = n + 1;

        for (;;) {
            unsigned char nc = *n2;
            if (nc == 0)
                return (char *)h;     /* full match */
            if (*h2 != nc)
                break;
            ++h2;
            ++n2;
        }
    }
}

inline static
char *strchr(const char *s, int c) {
    const unsigned char *p = (const unsigned char *)s;
    const unsigned char ch = (unsigned char)c;
    for (;;) {
        unsigned char v = *p;
        if (v == ch) return (char *)p;
        if (v == 0)  return 0;
        ++p;
    }
    __builtin_unreachable();
}

inline static
char* strncat(char* restrict dst, const char* restrict src, size_t n)
{
    char* d = dst;
    while (*d) d++;
    while (n-- && *src) {
        *d++ = *src++;
    }
    *d = '\0';
    return dst;
}

inline static
char* strrchr(const char* s, int c)
{
    const char* last = NULL;
    unsigned char ch = (unsigned char)c;
    for (; *s; s++) {
        if ((unsigned char)*s == ch) {
            last = s;
        }
    }
    if (ch == '\0') {
        return (char*)s;
    }
    return (char*)last;
}

// defined in runtime
void* memset(void* p, int v, size_t sz);
void* memcpy(void *__restrict dst, const void *__restrict src, size_t sz);
char* strcpy(char* t, const char * s);
void* memmove(void* dst, const void* src, size_t sz);
char* strcat(char* t, const char * s);

inline static size_t strlen(const char * s) {
    typedef size_t (*fn_ptr_t)(const char * s);
    return ((fn_ptr_t)_sys_table_ptrs[62])(s);
}

inline static size_t strnlen(const char *str, size_t sz) {
    typedef size_t (*fn_ptr_t)(const char *str, size_t sz);
    return ((fn_ptr_t)_sys_table_ptrs[170])(str, sz);
}

inline static char* strncpy(char* t, const char * s, size_t sz) {
    typedef char* (*fn_ptr_t)(char*, const char*, size_t);
    return ((fn_ptr_t)_sys_table_ptrs[63])(t, s, sz);
}

inline static int strcmp(const char * s1, const char * s2) {
    typedef int (*fn_ptr_t)(const char*, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[108])(s1, s2);
}

inline static int strncmp(const char * s1, const char * s2, size_t sz) {
    typedef int (*fn_ptr_t)(const char*, const char*, size_t);
    return ((fn_ptr_t)_sys_table_ptrs[109])(s1, s2, sz);
}

inline static char* strerror (int code) {
    typedef char* (*fn_ptr_t)(int);
    return ((fn_ptr_t)_sys_table_ptrs[353])(code);
}
// not standard functions
inline static char* concat(const char* s1, const char* s2) {
    typedef char* (*fn_ptr_t)(const char*, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[129])(s1, s2);
}

inline static void* malloc(size_t xWantedSize) {
    typedef void* (*pvPortMalloc_ptr_t)( size_t xWantedSize );
    return ((pvPortMalloc_ptr_t)_sys_table_ptrs[32])(xWantedSize);
}

inline static char *strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p)
        return 0;
    memcpy(p, s, n);
    return p;
}

#ifdef __cplusplus
}
#endif

#endif // _STRING_H
