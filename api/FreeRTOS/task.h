#ifndef M_OS_API_TASKS_H
#define M_OS_API_TASKS_H

#include "FreeRTOSConfig.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define _vTaskDelayPtrIdx 2
#define _xTaskDelayUntilPtrIdx 3
#define _xTaskAbortDelayPtrIdx 4
#define _uxTaskPriorityGetPtrIdx 5
#define _uxTaskPriorityGetFromISRPtrIdx 6
#define _uxTaskBasePriorityGetPtrIdx 7
#define _uxTaskBasePriorityGetFromISRPtrIdx 8
#define _eTaskGetStatePtrIdx 9
#define _vTaskGetInfoPtrIdx 10
#define _vTaskPrioritySetPtrIdx 11
#define _vTaskSuspendPtrIdx 12
#define _vTaskResumePtrIdx 13
#define _xTaskResumeFromISRPtrIdx 14
#define _vTaskSuspendAllPtrIdx 15
#define _xTaskResumeAllPtrIdx 16
#define _xTaskGetTickCountPtrIdx 17
#define _xTaskGetTickCountFromISRPtrIdx 18
#define _uxTaskGetNumberOfTasksPtrIdx 19
#define _pcTaskGetNamePtrIdx 20
#define _xTaskGetHandlePtrIdx 21
#define _uxTaskGetStackHighWaterMarkPtrIdx 22
#define _vTaskSetThreadLocalStoragePointerPtrIdx 23
#define _pvTaskGetThreadLocalStoragePointerPtrIdx 24


#define _getApplicationMallocFailedHookPtrPtrIdx 25
#define _setApplicationMallocFailedHookPtrPtrIdx 26
#define _getApplicationStackOverflowHookPtrPtrIdx 27
#define _setApplicationStackOverflowHookPtrPtrIdx 28

/*-----------------------------------------------------------
 * Port specific definitions.
 *
 * The settings in this file configure FreeRTOS correctly for the
 * given hardware and compiler.
 *
 * These settings should not be altered.
 *-----------------------------------------------------------
 */

/* Type definitions. */
#define portCHAR          char
#define portFLOAT         float
#define portDOUBLE        double
#define portLONG          long
#define portSHORT         short
#define portSTACK_TYPE    uint32_t
#define portBASE_TYPE     long

typedef portSTACK_TYPE   StackType_t;
typedef int32_t          BaseType_t;
typedef uint32_t         UBaseType_t;

#if ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
    typedef uint16_t     TickType_t;
    #define portMAX_DELAY              ( TickType_t ) 0xffff
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    typedef uint32_t     TickType_t;
    #define portMAX_DELAY              ( TickType_t ) 0xffffffffUL

/* 32-bit tick type on a 32-bit architecture, so reads of the tick count do
 * not need to be guarded with a critical section. */
    #define portTICK_TYPE_IS_ATOMIC    1
#else
    #error configTICK_TYPE_WIDTH_IN_BITS set to unsupported tick type width.
#endif
/*-----------------------------------------------------------*/

/* Architecture specifics. */
#define portSTACK_GROWTH                ( -1 )
#define portTICK_PERIOD_MS              ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT              8
#define portDONT_DISCARD                __attribute__( ( used ) )

/* We have to use PICO_DIVIDER_DISABLE_INTERRUPTS as the source of truth rathern than our config,
 * as our FreeRTOSConfig.h header cannot be included by ASM code - which is what this affects in the SDK */
#define portUSE_DIVIDER_SAVE_RESTORE    !PICO_DIVIDER_DISABLE_INTERRUPTS
#if portUSE_DIVIDER_SAVE_RESTORE
    #define portSTACK_LIMIT_PADDING     4
#endif


/* Converts a time in milliseconds to a time in ticks.  This macro can be
 * overridden by a macro of the same name defined in FreeRTOSConfig.h in case the
 * definition here is not suitable for your application. */
#ifndef pdMS_TO_TICKS
    #define pdMS_TO_TICKS( xTimeInMs )    ( ( TickType_t ) ( ( ( uint64_t ) ( xTimeInMs ) * ( uint64_t ) configTICK_RATE_HZ ) / ( uint64_t ) 1000U ) )
#endif

/* Converts a time in ticks to a time in milliseconds.  This macro can be
 * overridden by a macro of the same name defined in FreeRTOSConfig.h in case the
 * definition here is not suitable for your application. */
#ifndef pdTICKS_TO_MS
    #define pdTICKS_TO_MS( xTimeInTicks )    ( ( TickType_t ) ( ( ( uint64_t ) ( xTimeInTicks ) * ( uint64_t ) 1000U ) / ( uint64_t ) configTICK_RATE_HZ ) )
#endif

#define pdFALSE                                  ( ( BaseType_t ) 0 )
#define pdTRUE                                   ( ( BaseType_t ) 1 )
#define pdFALSE_SIGNED                           ( ( BaseType_t ) 0 )
#define pdTRUE_SIGNED                            ( ( BaseType_t ) 1 )
#define pdFALSE_UNSIGNED                         ( ( UBaseType_t ) 0 )
#define pdTRUE_UNSIGNED                          ( ( UBaseType_t ) 1 )

#define pdPASS                                   ( pdTRUE )
#define pdFAIL                                   ( pdFALSE )
#define errQUEUE_EMPTY                           ( ( BaseType_t ) 0 )
#define errQUEUE_FULL                            ( ( BaseType_t ) 0 )

/* FreeRTOS error definitions. */
#define errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY    ( -1 )
#define errQUEUE_BLOCKED                         ( -4 )
#define errQUEUE_YIELD                           ( -5 )

/* Macros used for basic data corruption checks. */
#ifndef configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES
    #define configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES    0
#endif

#if ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
    #define pdINTEGRITY_CHECK_VALUE    0x5a5a
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    #define pdINTEGRITY_CHECK_VALUE    0x5a5a5a5aUL
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_64_BITS )
    #define pdINTEGRITY_CHECK_VALUE    0x5a5a5a5a5a5a5a5aULL
#else
    #error configTICK_TYPE_WIDTH_IN_BITS set to unsupported tick type width.
#endif

/* The following errno values are used by FreeRTOS+ components, not FreeRTOS
 * itself. */
#define pdFREERTOS_ERRNO_NONE             0   /* No errors */
#define pdFREERTOS_ERRNO_ENOENT           2   /* No such file or directory */
#define pdFREERTOS_ERRNO_EINTR            4   /* Interrupted system call */
#define pdFREERTOS_ERRNO_EIO              5   /* I/O error */
#define pdFREERTOS_ERRNO_ENXIO            6   /* No such device or address */
#define pdFREERTOS_ERRNO_EBADF            9   /* Bad file number */
#define pdFREERTOS_ERRNO_EAGAIN           11  /* No more processes */
#define pdFREERTOS_ERRNO_EWOULDBLOCK      11  /* Operation would block */
#define pdFREERTOS_ERRNO_ENOMEM           12  /* Not enough memory */
#define pdFREERTOS_ERRNO_EACCES           13  /* Permission denied */
#define pdFREERTOS_ERRNO_EFAULT           14  /* Bad address */
#define pdFREERTOS_ERRNO_EBUSY            16  /* Mount device busy */
#define pdFREERTOS_ERRNO_EEXIST           17  /* File exists */
#define pdFREERTOS_ERRNO_EXDEV            18  /* Cross-device link */
#define pdFREERTOS_ERRNO_ENODEV           19  /* No such device */
#define pdFREERTOS_ERRNO_ENOTDIR          20  /* Not a directory */
#define pdFREERTOS_ERRNO_EISDIR           21  /* Is a directory */
#define pdFREERTOS_ERRNO_EINVAL           22  /* Invalid argument */
#define pdFREERTOS_ERRNO_ENOSPC           28  /* No space left on device */
#define pdFREERTOS_ERRNO_ESPIPE           29  /* Illegal seek */
#define pdFREERTOS_ERRNO_EROFS            30  /* Read only file system */
#define pdFREERTOS_ERRNO_EUNATCH          42  /* Protocol driver not attached */
#define pdFREERTOS_ERRNO_EBADE            50  /* Invalid exchange */
#define pdFREERTOS_ERRNO_EFTYPE           79  /* Inappropriate file type or format */
#define pdFREERTOS_ERRNO_ENMFILE          89  /* No more files */
#define pdFREERTOS_ERRNO_ENOTEMPTY        90  /* Directory not empty */
#define pdFREERTOS_ERRNO_ENAMETOOLONG     91  /* File or path name too long */
#define pdFREERTOS_ERRNO_EOPNOTSUPP       95  /* Operation not supported on transport endpoint */
#define pdFREERTOS_ERRNO_EAFNOSUPPORT     97  /* Address family not supported by protocol */
#define pdFREERTOS_ERRNO_ENOBUFS          105 /* No buffer space available */
#define pdFREERTOS_ERRNO_ENOPROTOOPT      109 /* Protocol not available */
#define pdFREERTOS_ERRNO_EADDRINUSE       112 /* Address already in use */
#define pdFREERTOS_ERRNO_ETIMEDOUT        116 /* Connection timed out */
#define pdFREERTOS_ERRNO_EINPROGRESS      119 /* Connection already in progress */
#define pdFREERTOS_ERRNO_EALREADY         120 /* Socket already connected */
#define pdFREERTOS_ERRNO_EADDRNOTAVAIL    125 /* Address not available */
#define pdFREERTOS_ERRNO_EISCONN          127 /* Socket is already connected */
#define pdFREERTOS_ERRNO_ENOTCONN         128 /* Socket is not connected */
#define pdFREERTOS_ERRNO_ENOMEDIUM        135 /* No medium inserted */
#define pdFREERTOS_ERRNO_EILSEQ           138 /* An invalid UTF-16 sequence was encountered. */
#define pdFREERTOS_ERRNO_ECANCELED        140 /* Operation canceled. */

/* The following endian values are used by FreeRTOS+ components, not FreeRTOS
 * itself. */
#define pdFREERTOS_LITTLE_ENDIAN          0
#define pdFREERTOS_BIG_ENDIAN             1

/* Re-defining endian values for generic naming. */
#define pdLITTLE_ENDIAN                   pdFREERTOS_LITTLE_ENDIAN
#define pdBIG_ENDIAN                      pdFREERTOS_BIG_ENDIAN



/*-----------------------------------------------------------
* MACROS AND DEFINITIONS
*----------------------------------------------------------*/

/*
 * If tskKERNEL_VERSION_NUMBER ends with + it represents the version in development
 * after the numbered release.
 *
 * The tskKERNEL_VERSION_MAJOR, tskKERNEL_VERSION_MINOR, tskKERNEL_VERSION_BUILD
 * values will reflect the last released version number.
 */
#define tskKERNEL_VERSION_NUMBER       "V11.0.1+"
#define tskKERNEL_VERSION_MAJOR        11
#define tskKERNEL_VERSION_MINOR        0
#define tskKERNEL_VERSION_BUILD        1

/* MPU region parameters passed in ulParameters
 * of MemoryRegion_t struct. */
#define tskMPU_REGION_READ_ONLY        ( 1UL << 0UL )
#define tskMPU_REGION_READ_WRITE       ( 1UL << 1UL )
#define tskMPU_REGION_EXECUTE_NEVER    ( 1UL << 2UL )
#define tskMPU_REGION_NORMAL_MEMORY    ( 1UL << 3UL )
#define tskMPU_REGION_DEVICE_MEMORY    ( 1UL << 4UL )

/* MPU region permissions stored in MPU settings to
 * authorize access requests. */
#define tskMPU_READ_PERMISSION         ( 1UL << 0UL )
#define tskMPU_WRITE_PERMISSION        ( 1UL << 1UL )

/* The direct to task notification feature used to have only a single notification
 * per task.  Now there is an array of notifications per task that is dimensioned by
 * configTASK_NOTIFICATION_ARRAY_ENTRIES.  For backward compatibility, any use of the
 * original direct to task notification defaults to using the first index in the
 * array. */
#define tskDEFAULT_INDEX_TO_NOTIFY     ( 0 )

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

/*
 * Defines the prototype to which the application task hook function must
 * conform.
 */
typedef BaseType_t (* TaskHookFunction_t)( void * arg );

/* Task states returned by eTaskGetState. */
typedef enum
{
    eRunning = 0, /* A task is querying the state of itself, so must be running. */
    eReady,       /* The task being queried is in a ready or pending ready list. */
    eBlocked,     /* The task being queried is in the Blocked state. */
    eSuspended,   /* The task being queried is in the Suspended state, or is in the Blocked state with an infinite time out. */
    eDeleted,     /* The task being queried has been deleted, but its TCB has not yet been freed. */
    eInvalid      /* Used as an 'invalid state' value. */
} eTaskState;

/* Actions that can be performed when vTaskNotify() is called. */
typedef enum
{
    eNoAction = 0,            /* Notify the task without updating its notify value. */
    eSetBits,                 /* Set bits in the task's notification value. */
    eIncrement,               /* Increment the task's notification value. */
    eSetValueWithOverwrite,   /* Set the task's notification value to a specific value even if the previous value has not yet been read by the task. */
    eSetValueWithoutOverwrite /* Set the task's notification value if the previous value has been read by the task. */
} eNotifyAction;

/*
 * Used internally only.
 */
typedef struct xTIME_OUT
{
    BaseType_t xOverflowCount;
    TickType_t xTimeOnEntering;
} TimeOut_t;

/*
 * Defines the memory ranges allocated to the task when an MPU is used.
 */
typedef struct xMEMORY_REGION
{
    void * pvBaseAddress;
    uint32_t ulLengthInBytes;
    uint32_t ulParameters;
} MemoryRegion_t;

/*
 * Defines the prototype to which task functions must conform.  Defined in this
 * file to ensure the type is known before portable.h is included.
 */
typedef void (* TaskFunction_t)( void * arg );

#ifndef portNUM_CONFIGURABLE_REGIONS
    #define portNUM_CONFIGURABLE_REGIONS    1
#endif

/*
 * Parameters required to create an MPU protected task.
 */
typedef struct xTASK_PARAMETERS
{
    TaskFunction_t pvTaskCode;
    const char * pcName;
    configSTACK_DEPTH_TYPE usStackDepth;
    void * pvParameters;
    UBaseType_t uxPriority;
    StackType_t * puxStackBuffer;
    MemoryRegion_t xRegions[ portNUM_CONFIGURABLE_REGIONS ];
    #if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )
        StaticTask_t * const pxTaskBuffer;
    #endif
} TaskParameters_t;

#ifndef configRUN_TIME_COUNTER_TYPE

/* Defaults to uint32_t for backward compatibility, but can be overridden in
 * FreeRTOSConfig.h if uint32_t is too restrictive. */

    #define configRUN_TIME_COUNTER_TYPE    uint32_t
#endif

/* Used with the uxTaskGetSystemState() function to return the state of each task
 * in the system. */
typedef struct xTASK_STATUS
{
    TaskHandle_t xHandle;                         /* The handle of the task to which the rest of the information in the structure relates. */
    const char * pcTaskName;                      /* A pointer to the task's name.  This value will be invalid if the task was deleted since the structure was populated! */
    UBaseType_t xTaskNumber;                      /* A number unique to the task. */
    eTaskState eCurrentState;                     /* The state in which the task existed when the structure was populated. */
    UBaseType_t uxCurrentPriority;                /* The priority at which the task was running (may be inherited) when the structure was populated. */
    UBaseType_t uxBasePriority;                   /* The priority to which the task will return if the task's current priority has been inherited to avoid unbounded priority inversion when obtaining a mutex.  Only valid if configUSE_MUTEXES is defined as 1 in FreeRTOSConfig.h. */
    configRUN_TIME_COUNTER_TYPE ulRunTimeCounter; /* The total run time allocated to the task so far, as defined by the run time stats clock.  See https://www.FreeRTOS.org/rtos-run-time-stats.html.  Only valid when configGENERATE_RUN_TIME_STATS is defined as 1 in FreeRTOSConfig.h. */
    StackType_t * pxStackBase;                    /* Points to the lowest address of the task's stack area. */
    #if ( ( portSTACK_GROWTH > 0 ) || ( configRECORD_STACK_HIGH_ADDRESS == 1 ) )
        StackType_t * pxTopOfStack;               /* Points to the top address of the task's stack area. */
        StackType_t * pxEndOfStack;               /* Points to the end address of the task's stack area. */
    #endif
    configSTACK_DEPTH_TYPE usStackHighWaterMark;  /* The minimum amount of stack space that has remained for the task since the task was created.  The closer this value is to zero the closer the task has come to overflowing its stack. */
    #if ( ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 ) )
        UBaseType_t uxCoreAffinityMask;           /* The core affinity mask for the task */
    #endif
} TaskStatus_t;

/* Possible return values for eTaskConfirmSleepModeStatus(). */
typedef enum
{
    eAbortSleep = 0,           /* A task has been made ready or a context switch pended since portSUPPRESS_TICKS_AND_SLEEP() was called - abort entering a sleep mode. */
    eStandardSleep,            /* Enter a sleep mode that will not last any longer than the expected idle time. */
    #if ( INCLUDE_vTaskSuspend == 1 )
        eNoTasksWaitingTimeout /* No tasks are waiting for a timeout so it is safe to enter a sleep mode that can only be exited by an external interrupt. */
    #endif /* INCLUDE_vTaskSuspend */
} eSleepModeStatus;

/**
 * Defines the priority used by the idle task.  This must not be modified.
 *
 * \ingroup TaskUtils
 */
#define tskIDLE_PRIORITY    ( ( UBaseType_t ) 0U )

/**
 * Defines affinity to all available cores.
 *
 * \ingroup TaskUtils
 */
#define tskNO_AFFINITY      ( ( UBaseType_t ) -1 )

/**
 * task. h
 *
 * Macro for forcing a context switch.
 *
 * \defgroup taskYIELD taskYIELD
 * \ingroup SchedulerControl
 */
#define taskYIELD()                          portYIELD()

/**
 * task. h
 *
 * Macro to mark the start of a critical code region.  Preemptive context
 * switches cannot occur when in a critical region.
 *
 * NOTE: This may alter the stack (depending on the portable implementation)
 * so must be used with care!
 *
 * \defgroup taskENTER_CRITICAL taskENTER_CRITICAL
 * \ingroup SchedulerControl
 */
#define taskENTER_CRITICAL()                 portENTER_CRITICAL()
#if ( configNUMBER_OF_CORES == 1 )
    #define taskENTER_CRITICAL_FROM_ISR()    portSET_INTERRUPT_MASK_FROM_ISR()
#else
    #define taskENTER_CRITICAL_FROM_ISR()    portENTER_CRITICAL_FROM_ISR()
#endif

/**
 * task. h
 *
 * Macro to mark the end of a critical code region.  Preemptive context
 * switches cannot occur when in a critical region.
 *
 * NOTE: This may alter the stack (depending on the portable implementation)
 * so must be used with care!
 *
 * \defgroup taskEXIT_CRITICAL taskEXIT_CRITICAL
 * \ingroup SchedulerControl
 */
#define taskEXIT_CRITICAL()                    portEXIT_CRITICAL()
#if ( configNUMBER_OF_CORES == 1 )
    #define taskEXIT_CRITICAL_FROM_ISR( x )    portCLEAR_INTERRUPT_MASK_FROM_ISR( x )
#else
    #define taskEXIT_CRITICAL_FROM_ISR( x )    portEXIT_CRITICAL_FROM_ISR( x )
#endif

/**
 * task. h
 *
 * Macro to disable all maskable interrupts.
 *
 * \defgroup taskDISABLE_INTERRUPTS taskDISABLE_INTERRUPTS
 * \ingroup SchedulerControl
 */
#define taskDISABLE_INTERRUPTS()    portDISABLE_INTERRUPTS()

/**
 * task. h
 *
 * Macro to enable microcontroller interrupts.
 *
 * \defgroup taskENABLE_INTERRUPTS taskENABLE_INTERRUPTS
 * \ingroup SchedulerControl
 */
#define taskENABLE_INTERRUPTS()     portENABLE_INTERRUPTS()

/* Definitions returned by xTaskGetSchedulerState().  taskSCHEDULER_SUSPENDED is
 * 0 to generate more optimal code when configASSERT() is defined as the constant
 * is used in assert() statements. */
#define taskSCHEDULER_SUSPENDED      ( ( BaseType_t ) 0 )
#define taskSCHEDULER_NOT_STARTED    ( ( BaseType_t ) 1 )
#define taskSCHEDULER_RUNNING        ( ( BaseType_t ) 2 )

/* Checks if core ID is valid. */
#define taskVALID_CORE_ID( xCoreID )    ( ( ( ( ( BaseType_t ) 0 <= ( xCoreID ) ) && ( ( xCoreID ) < ( BaseType_t ) configNUMBER_OF_CORES ) ) ) ? ( pdTRUE ) : ( pdFALSE ) )

inline static BaseType_t xTaskCreate( TaskFunction_t pxTaskCode,
                            const char * const pcName,
                            const configSTACK_DEPTH_TYPE uxStackDepth,
                            void * const pvParameters,
                            UBaseType_t uxPriority,
                            TaskHandle_t * const pxCreatedTask ) {
    typedef BaseType_t (*xTaskCreate_ptr_t)( TaskFunction_t pxTaskCode,
                            const char * const pcName,
                            const configSTACK_DEPTH_TYPE uxStackDepth,
                            void * const pvParameters,
                            UBaseType_t uxPriority,
                            TaskHandle_t * const pxCreatedTask );
    return ((xTaskCreate_ptr_t)_sys_table_ptrs[0])(
        pxTaskCode,
        pcName,
        uxStackDepth,
        pvParameters,
        uxPriority,
        pxCreatedTask
    );
}

typedef void (*vTaskDelay_ptr_t)( const TickType_t xTicksToDelay );
inline static void vTaskDelay( const TickType_t xTicksToDelay ) {
    ((vTaskDelay_ptr_t)_sys_table_ptrs[_vTaskDelayPtrIdx])( xTicksToDelay );
}
#define sleep_ms vTaskDelay

inline static void vTaskDelete( TaskHandle_t xTaskToDelete ) {
    typedef void (*vTaskDelete_ptr_t)( TaskHandle_t xTaskToDelete );    
    ((vTaskDelete_ptr_t)_sys_table_ptrs[1])(xTaskToDelete);
}

typedef void (*vApplicationMallocFailedHookPtr)( void );
typedef vApplicationMallocFailedHookPtr (*getApplicationMallocFailedHookPtr_ptr_t)();
typedef void (*setApplicationMallocFailedHookPtr_ptr_t)(vApplicationMallocFailedHookPtr ptr);
inline static vApplicationMallocFailedHookPtr getApplicationMallocFailedHookPtr() {
    return (vApplicationMallocFailedHookPtr)((getApplicationMallocFailedHookPtr_ptr_t)_sys_table_ptrs[_getApplicationMallocFailedHookPtrPtrIdx]);
}
inline static void setApplicationMallocFailedHookPtr(vApplicationMallocFailedHookPtr ptr) {
    ((setApplicationMallocFailedHookPtr_ptr_t)_sys_table_ptrs[_setApplicationMallocFailedHookPtrPtrIdx])(ptr);
}

typedef void (*vApplicationStackOverflowHookPtr)( TaskHandle_t pxTask, char *pcTaskName );
typedef vApplicationStackOverflowHookPtr (*getApplicationStackOverflowHookPtr_ptr_t)();
typedef void (*setApplicationStackOverflowHookPtr_ptr_t)(vApplicationStackOverflowHookPtr ptr);
inline static vApplicationStackOverflowHookPtr getApplicationStackOverflowHookPtr() {
    return ((getApplicationStackOverflowHookPtr_ptr_t)_sys_table_ptrs[_getApplicationStackOverflowHookPtrPtrIdx])();
}
inline static void setApplicationStackOverflowHookPtr(vApplicationStackOverflowHookPtr ptr) {
    ((setApplicationStackOverflowHookPtr_ptr_t)_sys_table_ptrs[_setApplicationStackOverflowHookPtrPtrIdx])(ptr);
}

inline static void* pvPortMalloc(size_t xWantedSize) {
    typedef void* (*pvPortMalloc_ptr_t)( size_t xWantedSize );
    return ((pvPortMalloc_ptr_t)_sys_table_ptrs[32])(xWantedSize);
}
inline static void vPortFree(void * pv) {
    typedef void (*vPortFree_ptr_t)( void * pv );
    ((vPortFree_ptr_t)_sys_table_ptrs[33])(pv);
}

inline static void vTaskSuspendAll( void ) {
    typedef void (*v_ptr_t)( void  );
    ((v_ptr_t)_sys_table_ptrs[_vTaskSuspendAllPtrIdx])();
}

inline static BaseType_t xTaskResumeAll( void ) {
    typedef BaseType_t (*v_ptr_t)( void  );
    return ((v_ptr_t)_sys_table_ptrs[_xTaskResumeAllPtrIdx])();
}

inline static UBaseType_t uxTaskGetNumberOfTasks( void ) {
    typedef UBaseType_t (*v_ptr_t)( void  );
    return ((v_ptr_t)_sys_table_ptrs[_uxTaskGetNumberOfTasksPtrIdx])();
}

inline static
UBaseType_t uxTaskGetSystemState( TaskStatus_t * const pxTaskStatusArray,
                                  const UBaseType_t uxArraySize,
                                  configRUN_TIME_COUNTER_TYPE * const pulTotalRunTime
) {
    typedef UBaseType_t (*v_ptr_t)( TaskStatus_t * const pxTaskStatusArray,
                                  const UBaseType_t uxArraySize,
                                  configRUN_TIME_COUNTER_TYPE * const pulTotalRunTime  );
    return ((v_ptr_t)_sys_table_ptrs[243])(pxTaskStatusArray, uxArraySize, pulTotalRunTime);
}

inline static void* pvTaskGetThreadLocalStoragePointer( TaskHandle_t xTaskToQuery, BaseType_t xIndex ) {
    typedef void* (*v_ptr_t)( TaskHandle_t, BaseType_t );
    return ((v_ptr_t)_sys_table_ptrs[_pvTaskGetThreadLocalStoragePointerPtrIdx])(xTaskToQuery, xIndex);
}

#endif // M_OS_API_TASKS_H
