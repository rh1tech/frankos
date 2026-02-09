#include "internal/pthread_impl.h"

#include "FreeRTOS.h"
#include "task.h"

#include "internal/stdio_impl.h"
#include "internal/__stdio.h"
#include "sys_table.h"

static inline int a_cas(volatile int *p, int expected, int newval)
{
    int old;
    taskENTER_CRITICAL();
    old = *p;
    if (old == expected)
        *p = newval;
    taskEXIT_CRITICAL();
    return old;
}

static inline void __futexwait(volatile int *addr, int expected, int priv)
{
    (void)priv;  // игнорируем
    for (int i = 0;;++i) {
        int cur;
        taskENTER_CRITICAL();
        cur = *addr;
        taskEXIT_CRITICAL();
        if (cur != expected)
            break;
		if (i % 32) taskYIELD();
		else vTaskDelay(1);
    }
}

static inline int a_swap(volatile int *p, int newval)
{
    int old;
    taskENTER_CRITICAL();
    old = *p;
    *p = newval;
    taskEXIT_CRITICAL();
    return old;
}

static inline void __wake(volatile int *addr, int n, int priv)
{
    (void)addr;
    (void)n;
    (void)priv;
    taskYIELD();
}

int __libc() __lockfile(FILE *f)
{
	int owner = f->lock;
	int tid = __pthread_tid();
	if ((owner & ~MAYBE_WAITERS) == tid)
		return 0;
	owner = a_cas(&f->lock, 0, tid);
	if (!owner) return 1;
	while ((owner = a_cas(&f->lock, 0, tid|MAYBE_WAITERS))) {
		if ((owner & MAYBE_WAITERS) || a_cas(&f->lock, owner, owner|MAYBE_WAITERS) == owner) {
			__futexwait(&f->lock, owner | MAYBE_WAITERS, 1);
		}
	}
	return 1;
}

void __libc() __unlockfile(FILE *f)
{
	if (a_swap(&f->lock, 0) & MAYBE_WAITERS)
		__wake(&f->lock, 1, 1);
}
