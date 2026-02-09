// /usr/include/unistd.h
#ifndef _UNISTD_H_
#define _UNISTD_H_

#ifdef __cplusplus
extern "C" {
#endif

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#include <stddef.h>

/**
 * Closes a file descriptor, releasing any resources associated with it.
 *
 * @param fildes
 *     File descriptor obtained from a successful call to open(), creat(), dup(), 
 *     pipe(), or similar. It must be a valid, open descriptor belonging to the calling process.
 *
 * @return
 *     On success: returns 0.
 *     On failure: returns -1 and sets errno to indicate the error.
 *
 * @errors
 *     EBADF  - The file descriptor 'fd' is not valid or not open.
 *     EINTR  - The call was interrupted by a signal before closing completed.
 *     EIO    - An I/O error occurred while flushing data to the underlying device.
 *
 * @notes
 *     - After a successful call, the file descriptor becomes invalid for future use.
 *     - Any further read(), write(), or lseek() calls on a closed descriptor will fail with EBADF.
 *     - If multiple descriptors refer to the same open file description (via dup() or fork()),
 *       the underlying file is closed only after all such descriptors have been closed.
 */
int __close(int fildes);
inline static int close(int fildes) {
    return __close(fildes);
}

/**
 * Reads from a file descriptor.
 *
 * @param fildes  File descriptor to read from.
 * @param buf     Pointer to the buffer to store read data.
 * @param count   Maximum number of bytes to read.
 *
 * @return
 *     On success: returns the number of bytes read (0 indicates EOF).
 *     On failure: returns -1 and sets errno.
 *
 * @errors
 *     EBADF  - 'fd' is not valid or not open for reading.
 *     EFAULT - 'buf' points outside accessible address space.
 *     EINTR  - Interrupted by signal.
 */
int __read(int fildes, void *buf, size_t count);

struct iovec {
    void  *iov_base;  /* pointer to data buffer */
    size_t iov_len;   /* length of buffer */
};

/**
 * readv - read data from a file descriptor into multiple buffers
 *
 * @fd:    the file descriptor to read from
 * @iov:   pointer to an array of iovec structures describing buffers
 * @iovcnt: number of elements in the iov array
 *
 * This function attempts to read data from the file descriptor @fd
 * into multiple buffers described by @iov. It performs a single
 * system call to read into all the buffers in order.  
 *
 * Returns:
 *   On success, returns the total number of bytes read (sum of all
 *   bytes placed into buffers).  
 *   On error, returns -1 and sets errno appropriately.
 *
 * Notes:
 *   - Partial reads are possible; fewer bytes than requested may
 *     be read.  
 *   - The iovec structure is defined as:
 *       struct iovec {
 *           void  *iov_base; // starting address of buffer
 *           size_t iov_len;  // length of buffer
 *       };
 */
int __readv(int fd, const struct iovec *iov, int iovcnt);

/**
 * Writes to a file descriptor.
 *
 * @param fildes  File descriptor to write to.
 * @param buf     Pointer to the buffer with data to write.
 * @param count   Number of bytes to write.
 *
 * @return
 *     On success: returns the number of bytes written.
 *     On failure: returns -1 and sets errno.
 *
 * @errors
 *     EBADF  - 'fd' is not valid or not open for writing.
 *     EFAULT - 'buf' points outside accessible address space.
 *     EINTR  - Interrupted by signal.
 */
int __write(int fildes, const void *buf, size_t count);

int __writev(int fd, const struct iovec *iov, int iovcnt);

/**
 * Duplicates a file descriptor
 *
 * @param oldfd  Existing file descriptor.
 *
 * @return
 *     On success: returns newfd.
 *     On failure: returns -1 and sets errno.
 *
 * @errors
 *     EBADF  - oldfd is not valid.
 *     EMFILE - newfd is out of available range.
 */
int __dup(int oldfd);

/**
 * Duplicates a file descriptor to a specified new descriptor.
 *
 * @param oldfd  Existing file descriptor.
 * @param newfd  Desired file descriptor number.
 *
 * @return
 *     On success: returns newfd.
 *     On failure: returns -1 and sets errno.
 *
 * @errors
 *     EBADF  - oldfd is not valid.
 *     EMFILE - newfd is out of available range.
 */
int __dup2(int oldfd, int newfd);
/// the same, but with flags preset (and oldfd == newfd -> EINVAL)
int __dup3(int oldfd, int newfd, int flags);

#define SEEK_SET 0  /* Set file offset to 'offset' bytes from the beginning */
#define SEEK_CUR 1  /* Set file offset to current position plus 'offset' */
#define SEEK_END 2  /* Set file offset to file size plus 'offset' */

/**
 * Repositions the file offset of an open file descriptor.
 *
 * @param fd
 *     The file descriptor referring to an open file. Must have been obtained 
 *     from open(), creat(), pipe(), or a similar function.
 *
 * @param offset
 *     The number of bytes to offset the file position. Its interpretation 
 *     depends on the value of 'whence':
 *       - SEEK_SET: set the file offset to 'offset' bytes from the beginning.
 *       - SEEK_CUR: set the file offset to its current location plus 'offset'.
 *       - SEEK_END: set the file offset to the size of the file plus 'offset'.
 *
 * @param whence
 *     One of SEEK_SET, SEEK_CUR, or SEEK_END, indicating how the offset should be applied.
 *
 * @return
 *     On success: returns the resulting file offset (non-negative).
 *     On failure: returns (off_t)-1 and sets errno to indicate the error.
 *
 * @errors
 *     EBADF  - 'fd' is not an open file descriptor.
 *     EINVAL - 'whence' is invalid, or resulting offset would be negative.
 *     EOVERFLOW - The resulting file offset cannot be represented in off_t.
 *     ESPIPE - 'fd' refers to a pipe, FIFO, or socket (which do not support seeking).
 *
 * @notes
 *     - The new file position affects subsequent read() and write() operations.
 *     - If the file is shared (via dup() or fork()), all descriptors that share
 *       the same open file description will see the new offset.
 *     - Seeking past the end of the file does not extend it until data is written.
 */
long __lseek_p(int fd, long offset, int whence);

/* Type for file offsets */
#ifndef _OFF_T_DECLARED
typedef long long off_t;
typedef long long _off_t;
#define __machine_off_t_defined
#define	_OFF_T_DECLARED
#endif

#ifndef _PID_T_DECLARED
typedef long pid_t;
typedef long _pid_t;
#define __machine_off_t_defined
#define	_PID_T_DECLARED
#endif

int __llseek(unsigned int fd,
             unsigned long offset_high,
             unsigned long offset_low,
             off_t *result,
             unsigned int whence);
#ifndef AT_FDCWD
#define AT_FDCWD (-100)
#endif
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_HLINK_NOFOLLOW 0x1000 // not standart (just for unlink)
#define AT_REMOVEDIR 0x200
#define AT_REMOVEANY 0x2000 // not standart (just for unlink)
#define AT_SYMLINK_FOLLOW 0x400
#define AT_EACCESS 0x200

int __renameat(int, const char *, int, const char *);
int __unlinkat(int dirfd, const char *pathname, int flags);

int __linkat(int, const char *, int, const char *, int);
int __symlinkat(const char *, int, const char *);

pid_t __fork(void);
int __execve(const char *pathname, char *const argv[], char *const envp[]);

int __fchdir(int);
int __chdir(const char* name);

pid_t __getpid(void);
pid_t __getppid(void);

pid_t __setsid(void);
pid_t __getsid(pid_t);

/* Type for user IDs */
#ifndef _UID_T_DECLARED
typedef unsigned int uid_t;
#define	_UID_T_DECLARED
#endif

/* Type for group IDs */
#ifndef _GID_T_DECLARED
typedef unsigned int gid_t;
#define	_GID_T_DECLARED
#endif

gid_t __getgid(void);
int __setgid(gid_t);

uid_t __getuid(void);
uid_t __geteuid(void);
gid_t __getgid(void);
gid_t __getegid(void);

int __setuid(uid_t);    // можно ENOSYS
int __seteuid(uid_t);   // можно ENOSYS
int __setgid(gid_t);    // можно ENOSYS
int __setegid(gid_t);   // можно ENOSYS

pid_t __getpgid(pid_t pid);
int   __setpgid(pid_t pid, pid_t pgid);

//pid_t getpgrp(void);      // = getpgid(0)
//int   setpgrp(void);      // = setpgid(0,0)

int __tcgetpgrp(int fd);
int __tcsetpgrp(int fd, pid_t pgrp);

#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1

int __access(const char *, int);

char* __getcwd(char *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* !_UNISTD_H_ */
