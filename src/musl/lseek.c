#include "sys/stat.h"
#include "unistd.h"
#include "sys_table.h"

off_t __libc() __lseek(int fd, off_t offset, int whence)
{
	off_t result;
	return __llseek(fd, (unsigned long)(offset >> 32), (unsigned long)offset, &result, whence) ? -1 : result;
}

weak_alias(__lseek, lseek);
