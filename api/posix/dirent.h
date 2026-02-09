#ifndef _DIRENT_H_
#define _DIRENT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef char TCHAR;
typedef unsigned int	UINT;	/* int must be 16-bit or 32-bit */
typedef unsigned char	BYTE;	/* char must be 8-bit */
typedef unsigned short	WORD;	/* 16-bit unsigned integer */
typedef unsigned int	DWORD;	/* 32-bit unsigned integer */
typedef unsigned long long QWORD;	/* 64-bit unsigned integer */
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

typedef struct {
	FFOBJID	obj;			/* Object identifier */
	DWORD	dptr;			/* Current read/write offset */
	DWORD	clust;			/* Current cluster */
	LBA_t	sect;			/* Current sector (0:Read operation has terminated) */
	BYTE*	dir;			/* Pointer to the directory item in the win[] */
	BYTE	fn[12];			/* SFN (in/out) {body[8],ext[3],status[1]} */
	DWORD	blk_ofs;		/* Offset of current entry block being processed (0xFFFFFFFF:Invalid) */
	void*	dirent;         // last used struct dirent* (or 0 - if not used)
} DIR;

// The type DIR represents a directory stream. The structure of the type DIR is unspecified.
inline static DIR* opendir(const char* path) {
    typedef DIR* (*fn_ptr_t)(const char*);
    return ((fn_ptr_t)_sys_table_ptrs[362])(path);
}

inline static DIR* opendirat(int bfd, const char* path) {
    typedef DIR* (*fn_ptr_t)(int, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[373])(bfd, path);
}

inline static int closedir(DIR* d) {
    typedef int (*fn_ptr_t)(DIR*);
    return ((fn_ptr_t)_sys_table_ptrs[363])(d);
}

struct dirent {
    char* d_name;
    FILINFO ff_info;
    int pos;
    size_t d_namlen;
};

inline static struct dirent* readdir(DIR* d) {
    typedef struct dirent* (*fn_ptr_t)(DIR*);
    return ((fn_ptr_t)_sys_table_ptrs[364])(d);
}

inline static void rewinddir(DIR *d) {
    typedef void (*fn_ptr_t)(DIR*);
    return ((fn_ptr_t)_sys_table_ptrs[365])(d);
}

inline static int dirfd(DIR* pd) {
    typedef int (*fn_ptr_t)(DIR*);
    return ((fn_ptr_t)_sys_table_ptrs[371])(pd);
}

#ifdef __cplusplus
}
#endif

#endif /* !_DIRENT_H_ */
