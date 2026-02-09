#ifndef M_OS_API_FF
#define M_OS_API_FF

#include <stdint.h>
#include <stddef.h>

#define f_error(fp) ((fp)->err)
#define f_tell(fp) ((fp)->fptr)
#define f_size(fp) ((fp)->obj.objsize)
#define f_rewind(fp) f_lseek((fp), 0)
#define f_rewinddir(dp) f_readdir((dp), 0)
#define f_rmdir(path) f_unlink(path)
#define f_unmount(path) f_mount(0, path, 0)

/* File attribute bits for directory entry (FILINFO.fattrib) */
#define	AM_RDO	0x01	/* Read only */
#define	AM_HID	0x02	/* Hidden */
#define	AM_SYS	0x04	/* System */
#define AM_DIR	0x10	/* Directory */
#define AM_ARC	0x20	/* Archive */

/* File function return code (FRESULT) */

typedef enum {
	FR_OK = 0,				/* (0) Succeeded */
	FR_DISK_ERR,			/* (1) A hard error occurred in the low level disk I/O layer */
	FR_INT_ERR,				/* (2) Assertion failed */
	FR_NOT_READY,			/* (3) The physical drive cannot work */
	FR_NO_FILE,				/* (4) Could not find the file */
	FR_NO_PATH,				/* (5) Could not find the path */
	FR_INVALID_NAME,		/* (6) The path name format is invalid */
	FR_DENIED,				/* (7) Access denied due to prohibited access or directory full */
	FR_EXIST,				/* (8) Access denied due to prohibited access */
	FR_INVALID_OBJECT,		/* (9) The file/directory object is invalid */
	FR_WRITE_PROTECTED,		/* (10) The physical drive is write protected */
	FR_INVALID_DRIVE,		/* (11) The logical drive number is invalid */
	FR_NOT_ENABLED,			/* (12) The volume has no work area */
	FR_NO_FILESYSTEM,		/* (13) There is no valid FAT volume */
	FR_MKFS_ABORTED,		/* (14) The f_mkfs() aborted due to any problem */
	FR_TIMEOUT,				/* (15) Could not get a grant to access the volume within defined period */
	FR_LOCKED,				/* (16) The operation is rejected according to the file sharing policy */
	FR_NOT_ENOUGH_CORE,		/* (17) LFN working buffer could not be allocated */
	FR_TOO_MANY_OPEN_FILES,	/* (18) Number of open files > FF_FS_LOCK */
	FR_INVALID_PARAMETER	/* (19) Given parameter is invalid */
} FRESULT;
typedef char TCHAR;
typedef unsigned int	UINT;	/* int must be 16-bit or 32-bit */
typedef unsigned char	BYTE;	/* char must be 8-bit */
typedef uint16_t		WORD;	/* 16-bit unsigned integer */
typedef uint32_t		DWORD;	/* 32-bit unsigned integer */
typedef uint64_t		QWORD;	/* 64-bit unsigned integer */
typedef WORD			WCHAR;	/* UTF-16 character type */


/* File information structure (FILINFO) */
typedef QWORD FSIZE_t;
#define FF_LFN_BUF		255
#define FF_SFN_BUF		12
typedef struct {
	FSIZE_t	fsize;			/* File size */
	WORD	fdate;			/* Modified date */
	WORD	ftime;			/* Modified time */
	BYTE	fattrib;		/* File attribute */
	TCHAR	altname[FF_SFN_BUF + 1];/* Altenative file name */
	TCHAR	fname[FF_LFN_BUF + 1];	/* Primary file name */
} FILINFO;

/* Filesystem object structure (FATFS) */
#define FF_MIN_SS		512
#define FF_MAX_SS		512
typedef DWORD LBA_t;

typedef struct {
	BYTE	fs_type;		/* Filesystem type (0:not mounted) */
	BYTE	pdrv;			/* Associated physical drive */
	BYTE	n_fats;			/* Number of FATs (1 or 2) */
	BYTE	wflag;			/* win[] flag (b0:dirty) */
	BYTE	fsi_flag;		/* FSINFO flags (b7:disabled, b0:dirty) */
	WORD	id;				/* Volume mount ID */
	WORD	n_rootdir;		/* Number of root directory entries (FAT12/16) */
	WORD	csize;			/* Cluster size [sectors] */
	WCHAR*	lfnbuf;			/* LFN working buffer */
	BYTE*	dirbuf;			/* Directory entry block scratchpad buffer for exFAT */
	DWORD	last_clst;		/* Last allocated cluster */
	DWORD	free_clst;		/* Number of free clusters */
	DWORD	n_fatent;		/* Number of FAT entries (number of clusters + 2) */
	DWORD	fsize;			/* Size of an FAT [sectors] */
	LBA_t	volbase;		/* Volume base sector */
	LBA_t	fatbase;		/* FAT base sector */
	LBA_t	dirbase;		/* Root directory base sector/cluster */
	LBA_t	database;		/* Data base sector */
	LBA_t	bitbase;		/* Allocation bitmap base sector */
	LBA_t	winsect;		/* Current sector appearing in the win[] */
	BYTE	win[FF_MAX_SS];	/* Disk access window for Directory, FAT (and file data at tiny cfg) */
} FATFS;

/* Object ID and allocation information (FFOBJID) */

typedef struct {
	FATFS*	fs;				/* Pointer to the hosting volume of this object */
	WORD	id;				/* Hosting volume mount ID */
	BYTE	attr;			/* Object attribute */
	BYTE	stat;			/* Object chain status (b1-0: =0:not contiguous, =2:contiguous, =3:fragmented in this session, b2:sub-directory stretched) */
	DWORD	sclust;			/* Object data start cluster (0:no cluster or root directory) */
	FSIZE_t	objsize;		/* Object size (valid when sclust != 0) */
	DWORD	n_cont;			/* Size of first fragment - 1 (valid when stat == 3) */
	DWORD	n_frag;			/* Size of last fragment needs to be written to FAT (valid when not zero) */
	DWORD	c_scl;			/* Containing directory start cluster (valid when sclust != 0) */
	DWORD	c_size;			/* b31-b8:Size of containing directory, b7-b0: Chain status (valid when c_scl != 0) */
	DWORD	c_ofs;			/* Offset in the containing directory (valid when file object and sclust != 0) */
} FFOBJID;

/* Directory object structure (DIR) */

typedef struct {
	FFOBJID	obj;			/* Object identifier */
	DWORD	dptr;			/* Current read/write offset */
	DWORD	clust;			/* Current cluster */
	LBA_t	sect;			/* Current sector (0:Read operation has terminated) */
	BYTE*	dir;			/* Pointer to the directory item in the win[] */
	BYTE	fn[12];			/* SFN (in/out) {body[8],ext[3],status[1]} */
	DWORD	blk_ofs;		/* Offset of current entry block being processed (0xFFFFFFFF:Invalid) */
} DIR;

/* File access mode and open method flags (3rd argument of f_open) */
#define	FA_READ				0x01
#define	FA_WRITE			0x02
#define	FA_OPEN_EXISTING	0x00
#define	FA_CREATE_NEW		0x04
#define	FA_CREATE_ALWAYS	0x08
#define	FA_OPEN_ALWAYS		0x10
#define	FA_OPEN_APPEND		0x30

typedef struct FIL {
	FFOBJID	obj;			/* Object identifier (must be the 1st member to detect invalid object pointer) */
	BYTE	flag;			/* File status flags */
	BYTE	err;			/* Abort flag (error code) */
	FSIZE_t	fptr;			/* File read/write pointer (Zeroed on file open) */
	DWORD	clust;			/* Current cluster of fpter (invalid when fptr is 0) */
	LBA_t	sect;			/* Sector number appearing in buf[] (0:invalid) */
	LBA_t	dir_sect;		/* Sector number containing the directory entry (not used at exFAT) */
	BYTE*	dir_ptr;		/* Pointer to the directory entry in the win[] (not used at exFAT) */
	DWORD*	cltbl;			/* Pointer to the cluster link map table (nulled on open, set by application) */
	BYTE	buf[FF_MAX_SS];	/* File private data read/write window */
    struct FIL* chained;
} FIL;

typedef FRESULT (*FRFcpCB_ptr_t)(FIL*, const TCHAR*, BYTE);
inline static FRESULT f_open (
	FIL* fp,			/* Pointer to the blank file object */
	const TCHAR* path,	/* Pointer to the file name */
	BYTE mode			/* Access mode and open mode flags */
) {
    return ((FRFcpCB_ptr_t)_sys_table_ptrs[46])(fp, path, mode);
}
typedef FRESULT (*FRF_ptr_t)(FIL*);
inline static FRESULT f_close (
	FIL* fp		/* Open file to be closed */
) {
    return ((FRF_ptr_t)_sys_table_ptrs[47])(fp);
}
typedef FRESULT (*FRFcpvUpU_ptr_t)(FIL*, const void*, UINT, UINT*);
inline static FRESULT f_write (
	FIL* fp,			/* Open file to be written */
	const void* buff,	/* Data to be written */
	UINT btw,			/* Number of bytes to write */
	UINT* bw			/* Number of bytes written */
) {
    return ((FRFcpvUpU_ptr_t)_sys_table_ptrs[48])(fp, buff, btw, bw);
}
typedef FRESULT (*FRFpvUpU_ptr_t)(FIL*, void*, UINT, UINT*);
inline static FRESULT f_read (
	FIL* fp, 	/* Open file to be read */
	void* buff,	/* Data buffer to store the read data */
	UINT btr,	/* Number of bytes to read */
	UINT* br	/* Number of bytes read */
) {
    return ((FRFpvUpU_ptr_t)_sys_table_ptrs[49])(fp, buff, btr, br);
}

typedef FRESULT (*FRcpCpFI_ptr_t)(const TCHAR*, FILINFO*);
inline static FRESULT f_stat (
	const TCHAR* path,	/* Pointer to the file path */
	FILINFO* fno		/* Pointer to file information to return */
) {
    return ((FRcpCpFI_ptr_t)_sys_table_ptrs[50])(path, fno);
}

typedef FRESULT (*FRpFFZ_ptr_t)(FIL*, FSIZE_t);
inline static FRESULT f_lseek (
	FIL* fp,		/* Pointer to the file object */
	FSIZE_t ofs		/* File pointer from top of file */
) {
    return ((FRpFFZ_ptr_t)_sys_table_ptrs[51])(fp, ofs);
}
inline static FRESULT f_truncate (
	FIL* fp		/* Pointer to the file object */
) {
    return ((FRF_ptr_t)_sys_table_ptrs[52])(fp);
}
inline static FRESULT f_sync (
	FIL* fp		/* Open file to be synced */
) {
    return ((FRF_ptr_t)_sys_table_ptrs[53])(fp);
}

inline static FRESULT f_opendir (
	DIR* dp,			/* Pointer to directory object to create */
	const TCHAR* path	/* Pointer to the directory path */
) {
    typedef FRESULT (*FRpDcpC_ptr_t)(DIR*, const TCHAR*);
    return ((FRpDcpC_ptr_t)_sys_table_ptrs[54])(dp, path);
}

typedef FRESULT (*FRpD_ptr_t)(DIR*);
inline static FRESULT f_closedir (
	DIR *dp		/* Pointer to the directory object to be closed */
) {
    return ((FRpD_ptr_t)_sys_table_ptrs[55])(dp);
}

inline static FRESULT f_readdir (
	DIR* dp,			/* Pointer to the open directory object */
	FILINFO* fno		/* Pointer to file information to return */
) {
    typedef FRESULT (*FRpDpFI_ptr_t)(DIR*, FILINFO*);
    return ((FRpDpFI_ptr_t)_sys_table_ptrs[56])(dp, fno);
}

typedef FRESULT (*FRcpC_ptr_t)(const TCHAR*);
inline static FRESULT f_mkdir (
	const TCHAR* path		/* Pointer to the directory path */
) {
    return ((FRcpC_ptr_t)_sys_table_ptrs[57])(path);
}
inline static FRESULT f_unlink (
	const TCHAR* path		/* Pointer to the file or directory path */
) {
    return ((FRcpC_ptr_t)_sys_table_ptrs[58])(path);
}

typedef FRESULT (*FRcpCcpC_ptr_t)(const TCHAR*, const TCHAR*);
inline static FRESULT f_rename (
	const TCHAR* path_old,	/* Pointer to the object name to be renamed */
	const TCHAR* path_new	/* Pointer to the new name */
) {
    return ((FRcpCcpC_ptr_t)_sys_table_ptrs[59])(path_old, path_new);
}

inline static FRESULT f_getfree (
	const TCHAR* path,	/* Logical drive number */
	DWORD* nclst,		/* Pointer to a variable to return number of free clusters */
	FATFS** fatfs		/* Pointer to return pointer to corresponding filesystem object */
) {
    typedef FRESULT (*fn_ptr_t)(const TCHAR*, DWORD*, FATFS**);
    return ((fn_ptr_t)_sys_table_ptrs[61])(path, nclst, fatfs);
}

inline static int f_getc (FIL* fp) {
    typedef int (*fn_ptr_t)(FIL* fp);
    return ((fn_ptr_t)_sys_table_ptrs[150])(fp);
}

inline static FRESULT f_open_pipe(FIL* to, FIL* from) {
    typedef FRESULT (*fn_ptr_t)(FIL*, FIL*);
    return ((fn_ptr_t)_sys_table_ptrs[151])(to, from);
}

inline static bool f_eof(FIL* fp) {
	return ((int)((fp)->fptr == (fp)->obj.objsize));
}

inline static bool f_read_str(FIL* f, char* buf, size_t lim) {
    typedef bool (*fn_ptr_t)(FIL* f, char* buf, size_t lim);
    return ((fn_ptr_t)_sys_table_ptrs[238])(f, buf, lim);
}

static size_t fwrite(const void* buf, size_t sz1, size_t sz2, FIL* file) {
    size_t res;
    f_write(file, buf, sz1 * sz2, &res);
    return res;
}
static size_t fread(void* buf, size_t sz1, size_t sz2, FIL* file) {
    size_t res;
    f_read(file, buf, sz1 * sz2, &res);
    return res;
}
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
static int fseek (FIL *stream, long offset, int origin) {
    if ( origin == SEEK_SET ) {
        return FR_OK != f_lseek(stream, offset);
    }
    if ( origin == SEEK_CUR ) {
        return FR_OK != f_lseek(stream, f_tell(stream) + offset);
    }
    if ( origin == SEEK_END ) {
        return FR_OK != f_lseek(stream, f_size(stream) + offset);
    }
    return 1;
}

#define ftell(f) f_tell(f)
#define fclose(f) f_close(f)
#define feof(f) f_eof(f)
#define remove(f) f_unlink(f)

#endif
