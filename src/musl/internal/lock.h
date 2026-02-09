#ifndef LOCK_H
#define LOCK_H

/// TODO:
#ifndef hidden
#define hidden __attribute__((__visibility__("hidden")))
#endif

hidden void __lock(volatile int *);
hidden void __unlock(volatile int *);
#define LOCK(x) __lock(x)
#define UNLOCK(x) __unlock(x)

#endif
