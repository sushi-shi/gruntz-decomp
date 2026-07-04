// StepList2.h - the shared coord recycle pool view (g_dropList / g_coordPool
// @0x645540) whose Drop (0x40163b, __thiscall on g_645540) recycles a node.
// Modeled NO-body so the call reloc-masks. Placeholder name; only the signature +
// emitted code bytes are load-bearing.
#ifndef GRUNTZ_GRUNTZ_CSTEPLIST2_H
#define GRUNTZ_GRUNTZ_CSTEPLIST2_H

#include <rva.h>

struct CStepList2 {
    void Drop(i32 node); // 0x40163b (__thiscall on g_645540)
};
SIZE_UNKNOWN(CStepList2);

#endif // GRUNTZ_GRUNTZ_CSTEPLIST2_H
