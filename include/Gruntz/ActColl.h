// ActColl.h - the shared coordinate-registry collection primitives reused by the
// whole engine's activation registries (the name registry, the per-class logic
// tables, and every leaf's activation-coordinate registry). Split out of
// <Gruntz/ActNameRegistry.h> so the CActReg archetype (<Gruntz/ActReg.h>) can reuse
// them WITHOUT pulling the bute-tree / MFC chain into the boundary-thunk pool.
//
// Find (0x16da80) is the slow coordinate lookup; Insert (0x16d850) rebuilds a slot;
// ActAlloc (0x16d990) hands out the alloc scratch. All external/no-body so the
// calls reloc-mask; g_actCache (0x6bf464) / g_actAllocResult (0x6bf428) are the
// shared alloc-scratch globals every registry reuses.
#ifndef GRUNTZ_GRUNTZ_ACTCOLL_H
#define GRUNTZ_GRUNTZ_ACTCOLL_H

#include <rva.h>

struct CActColl {
    i32 Find(i32 coord, i32 z); // 0x16da80 (__thiscall ret 8)
};
struct CActColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};
extern "C" i32 ActAlloc(); // 0x16d990

DATA(0x002bf464)
extern void* g_actCache;
DATA(0x002bf428)
extern void* g_actAllocResult;

#endif // GRUNTZ_GRUNTZ_ACTCOLL_H
