// /usr/include/sys/stat.h
#ifndef _SYS_STAT_H_
#define _SYS_STAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef AT_FDCWD
#define AT_FDCWD (-100)
#endif
#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_REMOVEDIR 0x200
#define AT_REMOVEANY 0x2000
#define AT_SYMLINK_FOLLOW 0x400
#define AT_EACCESS 0x200
#endif

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

/* Type for file modes (permissions) */
#ifndef _MODE_T_DECLARED
typedef unsigned int mode_t;
#define	_MODE_T_DECLARED
#endif

/* Type for device identifiers */
#ifndef _DEV_T_DECLARED
typedef unsigned long dev_t;
#define	_DEV_T_DECLARED
#endif

/* Type for inode numbers */
#ifndef _INO_T_DECLARED
typedef unsigned long ino_t;
#define	_INO_T_DECLARED
#endif

/* Type for number of hard links */
#ifndef _NLINK_T_DECLARED
typedef unsigned int nlink_t;
#define	_NLINK_T_DECLARED
#endif

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

/* Type for file offsets */
#ifndef _OFF_T_DECLARED
typedef long long off_t;
typedef long long _off_t;
#define __machine_off_t_defined
#define	_OFF_T_DECLARED
#endif

/* Type for time values (seconds since epoch) */
#ifndef _TIME_T_DECLARED
typedef long time_t;
#define	_TIME_T_DECLARED
#endif

/* File type mask */
#define S_IFMT   0170000  /* bitmask for file type */

/* File types */
#define S_IFREG  0100000  /* regular file */
#define S_IFDIR  0040000  /* directory */
#define S_IFCHR  0020000  /* character device */
#define S_IFBLK  0060000  /* block device */
#define S_IFIFO  0010000  /* FIFO / named pipe */
#define S_IFLNK  0120000  /* symbolic link */
#define S_IFSOCK 0140000  /* socket */

/* File permission bits: owner */
#define	S_IRWXU	0000700			/* RWX mask for owner */
#define S_IRUSR  0400  /* read permission, owner */
#define S_IWUSR  0200  /* write permission, owner */
#define S_IXUSR  0100  /* execute/search permission, owner */

/* File permission bits: group */
#define	S_IRWXG	0000070			/* RWX mask for group */
#define S_IRGRP  0040  /* read permission, group */
#define S_IWGRP  0020  /* write permission, group */
#define S_IXGRP  0010  /* execute/search permission, group */

/* File permission bits: others */
#define	S_IRWXO	0000007			/* RWX mask for other */
#define S_IROTH  0004  /* read permission, others */
#define S_IWOTH  0002  /* write permission, others */
#define S_IXOTH  0001  /* execute/search permission, others */

/* Test if something is a normal file.  */
#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

/* Test if something is a directory.  */
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

/* Test if something is a character special file.  */
#ifndef S_ISCHR
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#endif

/* Test if something is a block special file.  */
#ifndef S_ISBLK
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#endif

/* Test if something is a socket.  */
#ifndef S_ISSOCK
# ifdef S_IFSOCK
#   define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
# else
#   define S_ISSOCK(m) 0
# endif
#endif

/* Test if something is a FIFO.  */
#ifndef S_ISFIFO
# ifdef S_IFIFO
#  define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
# else
#  define S_ISFIFO(m) 0
# endif
#endif

/* Structure describing a file */
struct stat {
    dev_t     st_dev;     /* ID of device containing file */
    ino_t     st_ino;     /* inode number */
    mode_t    st_mode;    /* file type and mode (permissions) */
    nlink_t   st_nlink;   /* number of hard links */
    uid_t     st_uid;     /* user ID of owner */
    gid_t     st_gid;     /* group ID of owner */
    dev_t     st_rdev;    /* device ID (if special file) */
    off_t     st_size;    /* total size, in bytes */
    time_t    st_atime;   /* time of last access */
    time_t    st_mtime;   /* time of last modification */
    time_t    st_ctime;   /* time of last status change */
};

/**
* Retrieves information about a file or directory relative to a directory file descriptor.
*
* @param dirfd
  File descriptor referring to an open directory. If set to AT_FDCWD,
  'pathname' is interpreted relative to the current working directory.
*
* @param pathname
  Path to the target file or directory, relative to 'dirfd' if it is not absolute.
  May be NULL only if 'flags' includes AT_EMPTY_PATH and the target is specified
  by 'dirfd' itself.
* @param buf
  Pointer to a 'struct stat' object where file status information will be stored.
  Must not be NULL and must reference writable memory.
*
* @param flags
  Bitmask of options controlling behavior:
    - AT_SYMLINK_NOFOLLOW : Do not follow symbolic links.
    - AT_EMPTY_PATH       : If 'pathname' is empty, operate on the directory
                            referred to by 'dirfd' itself.
* @return
  On success: returns 0 and fills 'buf' with file status information.
  On failure: returns -1 and sets errno to indicate the specific error.
*
* @errors
  EACCES   - Permission denied to search a directory component of the path.
  ENOENT   - File or directory does not exist.
  ENOTDIR  - A component of the path prefix is not a directory.
  ELOOP    - Too many symbolic links encountered while resolving pathname.
  EBADF    - 'dirfd' is not a valid open file descriptor, or it does not refer to a directory
              when required.
  EINVAL   - Invalid flag specified in 'flags'.
  EFAULT   - 'pathname' or 'buf' points outside accessible address space.
*
* @notes
  - Equivalent to stat() when 'dirfd' is AT_FDCWD and 'flags' includes AT_SYMLINK_FOLLOW.
  - Use lstat() or AT_SYMLINK_NOFOLLOW to obtain information about the link itself
    rather than its target.
  - Use fstat() to obtain information about an already open file descriptor.
*/
inline static int fstatat(int dirfd, const char *pathname, struct stat *buf, int flags) {
    typedef int (*fn_ptr_t)(int, const char*, struct stat*, int);
    return ((fn_ptr_t)_sys_table_ptrs[267])(dirfd, pathname, buf, flags);
}
/**
 * Retrieves information about a file or directory specified by pathname.
 *
 * @param path
 *     Path to the file or directory. Can be absolute or relative.
 *
 * @param buf
 *     Pointer to a 'struct stat' object where file status information
 *     will be stored. Must not be NULL.
 *
 * @return
 *     On success: returns 0 and fills 'buf' with file information.
 *     On failure: returns -1 and sets errno to indicate the error.
 *
 * @errors
 *     EACCES   - Permission denied to access a component of the path.
 *     ENOENT   - File or directory does not exist.
 *     ENOTDIR  - A path component is not a directory.
 *     ELOOP    - Too many symbolic links encountered in resolving pathname.
 *     EFAULT   - 'buf' points outside accessible address space.
 *
 * @notes
 *     - 'buf->st_mode' contains the file type and permission bits.
 *     - 'buf->st_size' gives the size of the file in bytes.
 *     - 'buf->st_atime', 'buf->st_mtime', 'buf->st_ctime' represent
 *       last access, modification, and status change times.
 *     - This function does not follow symbolic links when using lstat().
 *     - Use fstat() for already opened file descriptors.
 */
inline static int stat(const char *path, struct stat *buf) {
    return fstatat(AT_FDCWD, path, buf, AT_SYMLINK_FOLLOW);
}

inline static int fstat(int fildes, struct stat *buf) {
    typedef int (*fn_ptr_t)(int, struct stat*);
    return ((fn_ptr_t)_sys_table_ptrs[268])(fildes, buf);
}

inline static int lstat(const char *path, struct stat *buf) {
    typedef int (*fn_ptr_t)(const char*, struct stat*);
    return ((fn_ptr_t)_sys_table_ptrs[269])(path, buf);
}

inline static int mkdirat(int dirfd, const char *pathname, mode_t mode) {
    typedef int (*fn_ptr_t)(int, const char*, mode_t);
    return ((fn_ptr_t)_sys_table_ptrs[356])(dirfd, pathname, mode);
}

inline static int mkdir(const char *pathname, mode_t mode) {
    return mkdirat(AT_FDCWD, pathname, mode);
}

inline static mode_t umask(mode_t mask) {
    typedef mode_t (*fn_ptr_t)(mode_t);
    return ((fn_ptr_t)_sys_table_ptrs[369])(mask);
}

inline static 
int fchmodat(int fd, const char* n, mode_t m, int fl) {
    typedef int (*fn_ptr_t)(int, const char*, mode_t, int);
    return ((fn_ptr_t)_sys_table_ptrs[374])(fd, n, m, fl);
}

inline static 
int fchmod(int d, mode_t m) {
    typedef int (*fn_ptr_t)(int, mode_t);
    return ((fn_ptr_t)_sys_table_ptrs[375])(d, m);
}

inline static 
int chmod(const char *n, mode_t m) {
  return fchmodat(AT_FDCWD, n, m, AT_SYMLINK_FOLLOW);
}

#ifdef __cplusplus
}
#endif

#endif /* !_SYS_STAT_H_ */
