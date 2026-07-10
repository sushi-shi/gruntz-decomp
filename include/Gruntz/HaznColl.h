// HaznColl.h - the single shared view of the engine coordinate/activation-registry
// collection (a _zvec-based registry). The SAME archetype is instantiated as the
// static-hazard registry (@0x64e3d0), the timebomb registry (@0x64c780), and the
// shared name registry (@0x6bf650), and range-registered from the boundary thunk
// pool. Find (0x16da80, __thiscall) is the slow coordinate lookup; RegisterRange
// (0x3742) seeds the fast [lo,hi] id range. Both are external/no-body so their calls
// reloc-mask. Placeholder name; offsets + code bytes are load-bearing.
//
// Previously duplicated as CTBombColl (TBombColl.h) and CHaznColl (HaznColl.h) - two
// byte-identical struct views of the one archetype, dual-binding g_nameReg@0x6bf650.
// Both folded into CCoordColl (the old names removed).
#ifndef GRUNTZ_GRUNTZ_CHAZNCOLL_H
#define GRUNTZ_GRUNTZ_CHAZNCOLL_H

#include <rva.h>

struct CCoordColl {
    i32 Find(i32 coord, i32 z);         // 0x16da80 (__thiscall ret 8)
    void RegisterRange(i32 lo, i32 hi); // 0x3742   (reloc-masked)
};

#endif // GRUNTZ_GRUNTZ_CHAZNCOLL_H
