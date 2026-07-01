// CTBombColl.h - the single shared view of the timebomb activation-registry
// collection (the same coordinate/activation-registry archetype as CHaznColl,
// instantiated as CTimeBomb's own registry @0x64c780 and the shared name registry
// @0x6bf650, and range-registered from the boundary thunk pool). Find (0x16da80,
// __thiscall) is the slow coordinate lookup; RegisterRange (0x3742) seeds the fast
// [lo,hi] id range. Both external/no-body so the calls reloc-mask. Placeholder
// name; offsets + code bytes are load-bearing.
#ifndef GRUNTZ_GRUNTZ_CTBOMBCOLL_H
#define GRUNTZ_GRUNTZ_CTBOMBCOLL_H

#include <rva.h>

struct CTBombColl {
    i32 Find(i32 coord, i32 z);         // 0x16da80 (__thiscall ret 8)
    void RegisterRange(i32 lo, i32 hi); // 0x3742   (reloc-masked)
};

#endif // GRUNTZ_GRUNTZ_CTBOMBCOLL_H
