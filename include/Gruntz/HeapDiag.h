#ifndef GRUNTZ_GRUNTZ_HEAPDIAG_H
#define GRUNTZ_GRUNTZ_HEAPDIAG_H

#include <Ints.h>

i32 FileExists(char* path);

struct tagMODULEENTRY32;
extern "C" i32 LegacyFindModule(u32 pid, u32 moduleId, struct tagMODULEENTRY32* out, u32 size);

namespace ApiCallerStubs {
    void winapi_118b50_OutputDebugStringA(i32 status);
}

#endif // GRUNTZ_GRUNTZ_HEAPDIAG_H
