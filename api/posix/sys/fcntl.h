// /usr/include/sys/fcntl.h
#ifndef _FCNTL_H_
#define	_FCNTL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

#ifndef _MODE_T_DECLARED
#define _MODE_T_DECLARED
typedef unsigned int mode_t;
#endif

/* File access modes */
#define O_RDONLY    0x0000  /* open for reading only */
#define O_WRONLY    0x0001  /* open for writing only */
#define O_RDWR      0x0002  /* open for reading and writing */

/* File creation and status flags */
#define O_CREAT     0x0040  /* create file if it does not exist */
#define O_EXCL      0x0080  /* error if O_CREAT and the file exists */
#define O_TRUNC     0x0200  /* truncate file to zero length */
#define O_APPEND    0x0400  /* append on each write */
#define O_NONBLOCK  0x0800  /* non-blocking mode */
#define O_SYNC      0x1000  /* write according to synchronized I/O file integrity completion */
#define O_NOFOLLOW  0x2000  /* do not follow symbolic links */

/* defined by POSIX Issue 7 */
#define	O_CLOEXEC	0x10000		/* atomically set FD_CLOEXEC */
#define	O_DIRECTORY	0x20000		/* fail if not a directory */

/* defined by POSIX Issue 8 */
#define	O_CLOFORK	0x40000		/* atomically set FD_CLOFORK */

#ifndef AT_FDCWD
#define AT_FDCWD (-100)
#endif
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_HLINK_NOFOLLOW 0x1000 // not standart (just for unlink)
#define AT_REMOVEDIR 0x200
#define AT_REMOVEANY 0x2000 // not standart (just for unlink)
#define AT_SYMLINK_FOLLOW 0x400
#define AT_EACCESS 0x200

/**
 * openat() — open a file relative to a directory file descriptor
 *
 * Parameters:
 *   dfd   – directory file descriptor (use AT_FDCWD for current directory)
 *   path  – pathname of the file to open
 *   flags – file status flags and access modes (see below)
 *   mode  – permissions to use if a new file is created
 *
 * flags (choose one):
 *   O_RDONLY – open for reading only
 *   O_WRONLY – open for writing only
 *   O_RDWR – open for both reading and writing
 * 
 * Additional flag options (combine with access mode using bitwise OR):
 *   O_CREAT – create the file if it does not exist (requires 'mode')
 *   O_EXCL – with O_CREAT, fail if the file already exists
 *   O_TRUNC – truncate file to zero length if it already exists
 *   O_APPEND – append writes to the end of file
 *   O_NONBLOCK – non-blocking I/O
 *   O_DIRECTORY – fail if the path is not a directory
 *   O_NOFOLLOW – do not follow symbolic links
 *   O_CLOEXEC – set close-on-exec (FD_CLOEXEC) on new descriptor
 *   O_SYNC – write operations wait for completion of file integrity updates
 *   O_DSYNC – write operations wait for data integrity completion
 *   O_TMPFILE – create an unnamed temporary file in the given directory (requires O_RDWR or O_WRONLY)
 * 
 * mode bits (used only with O_CREAT to define new file permissions):
 *   S_IRUSR – read permission for owner
 *   S_IWUSR – write permission for owner
 *   S_IXUSR – execute/search permission for owner
 *   S_IRGRP – read permission for group
 *   S_IWGRP – write permission for group
 *   S_IXGRP – execute/search permission for group
 *   S_IROTH – read permission for others
 *   S_IWOTH – write permission for others
 *   S_IXOTH – execute/search permission for others
 * 
 * Returns:
 *   On success: a new file descriptor (non-negative)
 *   On error:  -1 and errno is set appropriately
 */
inline static int openat(int dfd, const char *path, int oflag, mode_t mode) {
    typedef int (*fn_ptr_t)(int, const char*, int, mode_t);
    return ((fn_ptr_t)_sys_table_ptrs[265])(dfd, path, oflag, mode);
}

/**
 * Opens a file and returns a file descriptor for subsequent I/O operations.
 *
 * @param path
 *     Path to the file to be opened. Can be absolute or relative.
 *
 * @param oflag
 *     File access mode and options. Must include exactly one of:
 *       - O_RDONLY : open for reading only
 *       - O_WRONLY : open for writing only
 *       - O_RDWR   : open for reading and writing
 *     Additional flags may be combined using bitwise OR, such as:
 *       - O_CREAT  : create file if it does not exist (requires 'mode')
 *       - O_EXCL   : with O_CREAT, fail if file already exists
 *       - O_TRUNC  : truncate existing file to length 0
 *       - O_APPEND : append all writes to the end of file
 *       - O_NONBLOCK, O_SYNC, O_NOFOLLOW, etc.
 *
 * @param ...
 *     Optional argument of type mode_t, required if O_CREAT is specified.
 *     Defines the file's permissions (e.g., 0644), modified by the process umask.
 *
 * @return
 *     On success: non-negative file descriptor.
 *     On failure: -1 is returned and errno is set appropriately.
 *
 * @errors
 *     EACCES  - Permission denied.
 *     EEXIST  - File exists and O_CREAT|O_EXCL was used.
 *     ENOENT  - File does not exist and O_CREAT not specified.
 *     ENOTDIR - A path component is not a directory.
 *     EISDIR  - Tried to open a directory for writing.
 *     EMFILE  - Process limit of open files reached.
 *     ENFILE  - System-wide limit of open files reached.
 *     EINVAL  - Invalid flags.
 */
inline static int open(const char *path, int oflag, ...) {
    va_list ap;
    mode_t mode = 0;
    va_start(ap, oflag);
    /* mode only if O_CREAT */
    if (oflag & O_CREAT) {
        mode = va_arg(ap, mode_t);
    }
    va_end(ap);
    return openat(AT_FDCWD, path, oflag, mode);
}

/* Commands for fcntl() */
#define F_DUPFD         0   /* Duplicate file descriptor (>= arg) */
#define F_DUPFD_CLOEXEC 1030 /* Duplicate FD with FD_CLOEXEC */
#define F_GETFD         1   /* Get file descriptor flags */
#define F_SETFD         2   /* Set file descriptor flags */
#define F_GETFL         3   /* Get file status flags */
#define F_SETFL         4   /* Set file status flags */
#define F_GETLK         5   /* Get record locking information */
#define F_SETLK         6   /* Set record locking information (non-blocking) */
#define F_SETLKW        7   /* Set record locking information (blocking) */
#define F_GETOWN        8   /* Get owner (for SIGIO) */
#define F_SETOWN        9   /* Set owner (for SIGIO) */
#define F_GETSIG        10  /* Get signal for async notification */
#define F_SETSIG        11  /* Set signal for async notification */
#define F_SETLEASE      1024 /* Set file lease (Linux-specific) */
#define F_GETLEASE      1025 /* Get file lease */
#define F_NOTIFY        1026 /* Subscribe to filesystem events (Linux-specific) */

/* File descriptor flags */
#define FD_CLOEXEC      1   /* close-on-exec flag */

struct flock {
    short l_type;    /* F_RDLCK, F_WRLCK, F_UNLCK */
    short l_whence;  /* SEEK_SET, SEEK_CUR, SEEK_END */
    off_t l_start;   /* initial offset */
    off_t l_len;     /* len; 0 = fo EOF */
    pid_t l_pid;     /* ownner's PID (for F_GETLK) */
};

/*
 * fcntl() - perform various operations on a file descriptor
 *
 * Parameters:
 *   int fd       : file descriptor to operate on
 *   int cmd      : operation to perform. Common values include:
 *     F_DUPFD      : duplicate the file descriptor using the lowest available descriptor
 *                    greater than or equal to the third argument (int arg).
 *     F_DUPFD_CLOEXEC : like F_DUPFD, but sets the close-on-exec flag on the new descriptor.
 *     F_GETFD      : get the file descriptor flags. Returns the close-on-exec flag (FD_CLOEXEC).
 *     F_SETFD      : set the file descriptor flags. Uses int arg to set FD_CLOEXEC.
 *     F_GETFL      : get the file status flags and access modes (O_RDONLY, O_WRONLY, O_RDWR, O_NONBLOCK, etc.).
 *     F_SETFL      : set the file status flags. Uses int arg to set flags (O_APPEND, O_NONBLOCK, etc.).
 *     F_GETLK      : get the record locking information (advisory locks). Uses struct flock *arg.
 *     F_SETLK      : set or clear a record lock (non-blocking). Uses struct flock *arg.
 *     F_SETLKW     : set or clear a record lock (blocking). Uses struct flock *arg.
 *     F_GETOWN     : get the process ID or process group currently receiving SIGIO and SIGURG signals.
 *     F_SETOWN     : set the process ID or process group to receive SIGIO and SIGURG signals. Uses int arg.
 *     F_GETSIG     : get the signal sent when I/O is possible (Linux-specific).
 *     F_SETSIG     : set the signal sent when I/O is possible (Linux-specific). Uses int arg.
 *   ...          : optional argument depending on cmd (int or struct flock* usually)
 *
 * Returns:
 *   - On success: a non-negative value (meaning depends on cmd)
 *   - On error  : -1 and sets errno appropriately
 *
 * Errors (examples):
 *   EBADF   : fd is not a valid file descriptor
 *   EINVAL  : cmd is invalid or argument is inappropriate
 *   ENOLCK  : cannot acquire lock (for F_SETLK/F_SETLKW)
 */
inline static int fcntl(int fd, int cmd, ...) {
    va_list ap;
    uintptr_t a3 = 0;
    va_start(ap, cmd);
    switch (cmd) {
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            a3 = (uintptr_t)va_arg(ap, struct flock *);
            break;
        case F_SETFL:
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETOWN:
        case F_SETFD: {
            a3 = (uintptr_t)va_arg(ap, int);
            break;
        }
    }
    va_end(ap);
    typedef int (*fn_ptr_t)(int, int, uintptr_t);
    return ((fn_ptr_t)_sys_table_ptrs[272])(fd, cmd, a3);
}

#ifdef __cplusplus
}
#endif

#endif /* !_FCNTL_H_ */
