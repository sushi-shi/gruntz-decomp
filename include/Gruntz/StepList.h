// CStepList.h - the shared CObList step-list view whose RemoveAll (0x1b48a6 /
// VA 0x5b48a6, __thiscall) clears it. Modeled NO-body so the call reloc-masks.
// Placeholder name; only the signature + emitted code bytes are load-bearing.
#ifndef GRUNTZ_GRUNTZ_CSTEPLIST_H
#define GRUNTZ_GRUNTZ_CSTEPLIST_H

#include <rva.h>

struct CStepList {
    void RemoveAll(); // 0x5b48a6 (__thiscall)
};

#endif // GRUNTZ_GRUNTZ_CSTEPLIST_H
