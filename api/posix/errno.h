#ifndef _ERRNO_H_
#define _ERRNO_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Standard POSIX error codes */
#define EPERM     1   /* Operation not permitted */
#define ENOENT    2   /* No such file or directory */
#define ESRCH     3   /* No such process */
#define EINTR     4   /* Interrupted system call */
#define EIO       5   /* I/O error */
#define ENOEXEC          8
#define EBADF     9   /* Bad file descriptor */
#define ECHILD          10
#define	EAGAIN    11  /* No more processes */
#define ENOMEM    12  /* Not enough memory */
#define EACCES    13  /* Permission denied */
#define EFAULT    14  /* Bad address: pointer points outside accessible memory */
#define EBUSY     16
#define EEXIST    17  /* File exists */
#define ENOTDIR   20  /* Not a directory */
#define EISDIR    21  /* Is a directory */
#define EINVAL    22  /* Invalid argument */
#define ENFILE    23  /* Too many open files in system */
#define ENOTTY          25
#define EMFILE    24  /* Too many open files per process */
#define ENOSPC    28  /* No space left on device */
#define ESPIPE    29  /* Illegal seek */
#define ERANGE    34
#define ENAMETOOLONG    36
#define ENOSYS          38
#define ELOOP     40
#define EOVERFLOW 75  /* Value too large for defined data type */
#define EOPNOTSUPP      95
#define ENOTSUP         EOPNOTSUPP

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

/* You can define other standard POSIX errors here as needed */
inline static int* __errno_location() {
    typedef int* (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[276])();
}
#define errno (*__errno_location())

#ifdef __cplusplus
}
#endif

#endif /* _ERRNO_H_ */
