// HeapDiag.h - the HeapDiag.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_HEAPDIAG_H
#define GRUNTZ_GRUNTZ_HEAPDIAG_H

#include <Ints.h>

i32 FileExists(char* path);

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
struct tagMODULEENTRY32; // tlhelp32.h (consumer TUs include it)
extern "C" i32 LegacyFindModule(u32 pid, u32 moduleId, struct tagMODULEENTRY32* out, u32 size);


// File-scope prototypes moved from the .cpp (external linkage
// belongs in the owner header).

namespace ApiCallerStubs {
    void winapi_118b50_OutputDebugStringA(i32 status);
} // namespace ApiCallerStubs


#endif // GRUNTZ_GRUNTZ_HEAPDIAG_H
