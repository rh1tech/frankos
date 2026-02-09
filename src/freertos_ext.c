/*
 * FreeRTOS extensions for MOS2 compatibility.
 * Provides pvPortRealloc which MOS2's musl layer requires
 * but standard FreeRTOS Heap4 doesn't include.
 */
#include "FreeRTOS.h"
#include "portable.h"
#include <string.h>

void *pvPortRealloc(void *ptr, size_t new_size)
{
    if (ptr == NULL) return pvPortMalloc(new_size);
    if (new_size == 0) {
        vPortFree(ptr);
        return NULL;
    }
    void *new_ptr = pvPortMalloc(new_size);
    if (new_ptr == NULL) return NULL;
    memcpy(new_ptr, ptr, new_size);
    vPortFree(ptr);
    return new_ptr;
}
