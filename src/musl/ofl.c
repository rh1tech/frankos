#include "internal/stdio_impl.h"
#include "internal/__stdio.h"
#include "internal/lock.h"
#include "sys_table.h"

static FILE *ofl_head = 0;
static volatile int ofl_lock[1];
volatile int *const __stdio_ofl_lockptr = ofl_lock;

// actually only one CPU core, TODO: implemnet it, if both to be used
void __libc() __lock(volatile int *l) {
}

void __libc() __unlock(volatile int *l) {
}

FILE ** __libc() __ofl_lock()
{
	LOCK(ofl_lock);
	return &ofl_head;
}

void __libc() __ofl_unlock()
{
	UNLOCK(ofl_lock);
}
