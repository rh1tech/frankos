#ifndef _SIGNAL_H
#define _SIGNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

#include <stddef.h>
#include <stdint.h>

/* Тип для атомарных операций с сигналами (C) */
typedef int sig_atomic_t;

/* Маска сигналов (одно машинное слово — достаточно под 32 сигнала) */
typedef uint32_t sigset_t;

/* Тип обработчика сигнала (POSIX / C) */
typedef void (*sighandler_t)(int);

/* Специальные значения обработчиков (C/POSIX) */
#define SIG_DFL ((sighandler_t)0)    /* обработчик по умолчанию */
#define SIG_IGN ((sighandler_t)1)    /* игнорировать сигнал    */
#define SIG_ERR ((sighandler_t)-1)   /* ошибка из signal()     */

/* ===== Номера сигналов =====
 * POSIX не фиксирует значения, но обычно (как на Linux) так:
 * это ИМПЛЕМЕНТАЦИОННЫЙ ВЫБОР, можно взять такой набор.
 */

#define SIGHUP      1   /* hangup */
#define SIGINT      2   /* interrupt (Ctrl+C) */
#define SIGQUIT     3
#define SIGILL      4
#define SIGABRT     6
#define SIGFPE      8
#define SIGKILL     9
#define SIGSEGV     11
#define SIGPIPE     13
#define SIGALRM     14
#define SIGTERM     15
#define SIGCHLD     17

#define MAX_SIG     32

#define SIG_BLOCK     0
#define SIG_UNBLOCK   1
#define SIG_SETMASK   2

#define DEFAULT_MASK ((1u << SIGINT) | (1u << SIGTERM) | (1u << SIGKILL))

inline static
int kill(pid_t pid, int sig) {
    typedef int (*fn_ptr_t)(pid_t, int);
    return ((fn_ptr_t)_sys_table_ptrs[399])(pid, sig);
}

inline static
sighandler_t signal(int sig, sighandler_t handler) {
    typedef sighandler_t (*fn_ptr_t)(int, sighandler_t);
    return ((fn_ptr_t)_sys_table_ptrs[400])(sig, handler);
}

inline static
int raise(int sig) {
    typedef int (*fn_ptr_t)(int);
    return ((fn_ptr_t)_sys_table_ptrs[401])(sig);
}

inline static
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    typedef int (*fn_ptr_t)(int, const sigset_t*, sigset_t*);
    return ((fn_ptr_t)_sys_table_ptrs[402])(how, set, oldset);
}

#ifdef __cplusplus
}
#endif

#endif /* _SIGNAL_H */
