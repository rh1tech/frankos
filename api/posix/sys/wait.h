#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include <sys/types.h>   /* for size_t, ssize_t, etc. */

/* Begin extern "C" if C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

#define WNOHANG    1
#define WUNTRACED  2

#define WSTOPPED   2
#define WEXITED    4
#define WCONTINUED 8
#define WNOWAIT    0x1000000

typedef enum {
	P_ALL = 0,
	P_PID = 1,
	P_PGID = 2,
	P_PIDFD = 3
} idtype_t;

#ifndef _PID_T_DECLARED
typedef long pid_t;
typedef long _pid_t;
#define __machine_off_t_defined
#define	_PID_T_DECLARED
#endif

inline static
pid_t waitpid (pid_t pid, int* pstatus, int options) {
    typedef pid_t (*fn_ptr_t)(pid_t, int *, int);
    return ((fn_ptr_t)_sys_table_ptrs[380])(pid, pstatus, options);
}

inline static
pid_t wait (int* status) {
	return waitpid(-1, status, 0);
}

/* End extern "C" if C++ */
#ifdef __cplusplus
}
#endif

#endif /* _SYS_WAIT_H */
