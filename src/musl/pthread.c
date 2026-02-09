#include "FreeRTOS.h"
#include "task.h"

#include "internal/pthread_impl.h"
#include "sys_table.h"

int __libc() __pthread_tid() {
    return (intptr_t)xTaskGetCurrentTaskHandle() + 1;
}
