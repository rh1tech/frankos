// /usr/include/sys/stat.h
#ifndef _SYS_STAT_H_
#define _SYS_STAT_H_

#ifdef __cplusplus
extern "C" {
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
#define S_IRUSR  0400  /* read permission, owner */
#define S_IWUSR  0200  /* write permission, owner */
#define S_IXUSR  0100  /* execute/search permission, owner */

/* File permission bits: group */
#define S_IRGRP  0040  /* read permission, group */
#define S_IWGRP  0020  /* write permission, group */
#define S_IXGRP  0010  /* execute/search permission, group */

/* File permission bits: others */
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

#ifndef S_ISLNK
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
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
    time_t    st_mtime;   /* time of last modification (content) */
    time_t    st_ctime;   /* time of last status (metadata) or content change */
};

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
int __stat(const char *path, struct stat *buf);
int __fstat(int fildes, struct stat *buf);
int __lstat(const char *path, struct stat *buf); // for symbolic links
int __fstatat(int dirfd, const char *pathname, struct stat *buf, int flags);
int __mkdirat(int dirfd, const char *pathname, mode_t mode);
int __rmdirat(int dirfd, const char* pathname);
mode_t __umask(mode_t mask);
int __fchmodat(int fd, const char* n, mode_t m, int fl);
int __fchmod(int d, mode_t m);

#ifdef __cplusplus
}
#endif

#endif /* !_SYS_STAT_H_ */
