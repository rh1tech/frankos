#ifndef _DIRENT_H_
#define _DIRENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ff.h"

// The type DIR represents a directory stream. The structure of the type DIR is unspecified.
DIR* __opendir(const char*);
DIR* __opendirat(int bfd, const char* _path);
int __closedir(DIR *);
void __rewinddir(DIR *);

struct dirent {
    char* d_name;
    FILINFO ff_info;
    int pos;
    size_t d_namlen;
};

struct dirent* __readdir(DIR *);

int __dirfd(DIR *);

#ifdef __cplusplus
}
#endif

#endif /* !_DIRENT_H_ */
