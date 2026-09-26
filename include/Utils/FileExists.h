#ifndef UTILS_FILEEXISTS_H
#define UTILS_FILEEXISTS_H

#include <Ints.h>

// Defined, not declared: Gruntz/Utils.cpp includes it at file scope for the shared
// copy; a TU that needs a private copy includes it inside `namespace {}`.
i32 FileExists(const char* szPath) {
    OFSTRUCT of;

    if (!szPath) {
        return 0;
    }
    if (!*szPath) {
        return 0;
    }
    return OpenFile(szPath, &of, 0x4000) != -1;
}

#endif // UTILS_FILEEXISTS_H
