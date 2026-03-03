/*
 * OPL library — FreeRTOS heap allocation redirect
 *
 * The pico-sdk's __wrap_malloc panics on allocation failure instead of
 * returning NULL.  The FreeRTOS heap (pvPortMalloc) is much larger and
 * returns NULL on OOM, letting callers handle it gracefully.
 *
 * Include this header AFTER <stdlib.h> in every OPL source file that
 * allocates memory.
 */
#ifndef OPL_ALLOC_H
#define OPL_ALLOC_H

#include "FreeRTOS.h"
#include "portable.h"

#undef malloc
#undef calloc
#undef free
#undef realloc
#define malloc(sz)      pvPortMalloc(sz)
#define calloc(n, sz)   pvPortCalloc(n, sz)
#define free(p)         vPortFree(p)

extern void *pvPortRealloc(void *ptr, size_t new_size);
#define realloc(p, sz)  pvPortRealloc(p, sz)

#endif /* OPL_ALLOC_H */
