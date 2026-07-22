// HeapDiag.h - the HeapDiag.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_HEAPDIAG_H
#define GRUNTZ_GRUNTZ_HEAPDIAG_H

#include <Ints.h>

i32 FileExists(char* path);

i32 FileExists(const char* p); // 0x404282

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
struct tagMODULEENTRY32; // tlhelp32.h (consumer TUs include it)
extern "C" i32 LegacyFindModule(u32 pid, u32 moduleId, struct tagMODULEENTRY32* out, u32 size);

#endif // GRUNTZ_GRUNTZ_HEAPDIAG_H
