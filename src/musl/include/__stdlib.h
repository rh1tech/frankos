#ifndef _STDLIB_H
#define _STDLIB_H

#ifdef __cplusplus
extern "C" {
#endif

void* __new_ctx(void);

void* __malloc(size_t sz);
void* __calloc(size_t n, size_t sz);
void* __realloc(void* p, size_t sz);
void __free(void* p);

void* __malloc2(void* ctx, size_t sz);
void* __calloc2(void* ctx, size_t n, size_t sz);
void* __realloc2(void* ctx, void* p, size_t sz);
void __free2(void* ctx, void* p);
void __free_ctx(void* ctx);

char* __copy_str(const char* s);

#define malloc(sz) __malloc(sz)
#define calloc(sz0, sz1) __calloc(sz0, sz1)
#define free(p) __free(p)
#define realloc(ptr, new_size) __realloc(ptr, new_size)
/// TODO:
#define memalign(alignment, size) __malloc(alignment, size)
#define aligned_alloc(alignment, size) _malloc(alignment, size)

char* __getenv (const char *v);

#ifdef __cplusplus
}
#endif

#endif
