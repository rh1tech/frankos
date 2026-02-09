#ifndef __system_rtos_h__
#define __system_rtos_h__

#include <stdint.h>

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

/**
 * task. h
 *
 * Type by which tasks are referenced.  For example, a call to xTaskCreate
 * returns (via a pointer parameter) an TaskHandle_t variable that can then
 * be used as a parameter to vTaskDelete to delete the task.
 *
 * \defgroup TaskHandle_t TaskHandle_t
 * \ingroup Tasks
 */
struct tskTaskControlBlock; /* The old naming convention is used to prevent breaking kernel aware debuggers. */
typedef struct tskTaskControlBlock         * TaskHandle_t;
typedef const struct tskTaskControlBlock   * ConstTaskHandle_t;

//#define _xTaskDelayUntilPtrIdx 3
//#define _xTaskAbortDelayPtrIdx 4

inline static void vTaskDelay( const uint32_t xTicksToDelay ) {
    #define _vTaskDelayPtrIdx 2
    typedef void (*vTaskDelay_ptr_t)( const uint32_t xTicksToDelay );
    ((vTaskDelay_ptr_t)_sys_table_ptrs[_vTaskDelayPtrIdx])( xTicksToDelay );
}

inline static size_t xPortGetFreeHeapSize( void ) {
    typedef size_t (*_ptr_t)(void);
    return ((_ptr_t)_sys_table_ptrs[145])();
}

#endif // __system_rtos_h__
