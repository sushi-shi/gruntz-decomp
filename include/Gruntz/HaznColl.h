// HaznColl.h - the single shared view of the coordinate/activation-registry
// collection. The same engine collection archetype is instantiated as the
// static-hazard registry (@0x64e3d0) and the shared name registry (@0x6bf650), and
// range-registered from the boundary thunk pool. Find (0x16da80, __thiscall) is the
// slow coordinate lookup; RegisterRange (0x3742) seeds the fast [lo,hi] id range.
// Both are external/no-body so their calls reloc-mask. Placeholder name; offsets +
// code bytes are load-bearing.
#ifndef GRUNTZ_GRUNTZ_CHAZNCOLL_H
#define GRUNTZ_GRUNTZ_CHAZNCOLL_H

#include <rva.h>

struct CHaznColl {
    i32 Find(i32 coord, i32 z);         // 0x16da80 (__thiscall ret 8)
    void RegisterRange(i32 lo, i32 hi); // 0x3742   (reloc-masked)
};

#endif // GRUNTZ_GRUNTZ_CHAZNCOLL_H
