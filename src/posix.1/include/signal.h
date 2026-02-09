#ifndef _SIGNAL_H
#define _SIGNAL_H

#ifdef __cplusplus
extern "C" {
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

int __kill(pid_t pid, int sig);
sighandler_t __signal(int sig, sighandler_t handler);
#undef __raise;
int __raise(int sig);
int __sigprocmask(int how, const sigset_t *set, sigset_t *oldset);

#ifdef __cplusplus
}
#endif

#endif /* _SIGNAL_H */
