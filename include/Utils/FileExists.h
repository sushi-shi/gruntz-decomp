#ifndef UTILS_FILEEXISTS_H
#define UTILS_FILEEXISTS_H

#include <Ints.h>

i32 FileExists(const char* szPath) {
    OFSTRUCT of;

    if (!szPath) {
        return 0;
    }
    if (!*szPath) {
        return 0;
    }
    return OpenFile(szPath, &of, OF_EXIST) != HFILE_ERROR;
}

#endif
