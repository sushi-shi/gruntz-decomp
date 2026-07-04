// ActColl.h - the shared coordinate-registry collection primitives reused by the
// whole engine's activation registries (the name registry, the per-class logic
// tables, and every leaf's activation-coordinate registry). Split out of
// <Gruntz/ActNameRegistry.h> so the CActReg archetype (<Gruntz/ActReg.h>) can reuse
// them WITHOUT pulling the bute-tree / MFC chain into the boundary-thunk pool.
//
// Find (0x16da80) is the slow coordinate lookup; Insert (0x16d850) rebuilds a slot;
// GetRetAddr (0x16d990) is NOT an allocator - it is `pop eax;push eax;ret`, returning
// the caller's call-site return address, which the resolve stamps into g_retAddrBreadcrumb
// (0x6bf428) as an error/diagnostic breadcrumb right before the Insert (the misnomer
// "ActAlloc"/"ProjActAlloc" is corrected to GetRetAddr; see the report). All
// external/no-body so the calls reloc-mask; g_actCache (0x6bf464) is the real shared
// scratch buffer Insert consumes.
#ifndef GRUNTZ_GRUNTZ_ACTCOLL_H
#define GRUNTZ_GRUNTZ_ACTCOLL_H

#include <rva.h>

// The coordinate collection itself: its first dword is the collection object (a
// zDArray-family vtable/base). Every registry IS-A CActColl at +0x00 (CActReg
// derives from it), so the slow Find lookup is a direct base call - no view cast.
struct CActColl {
    void* m_coll;                   // +0x00  the collection object (its first dword)
    i32 Find(i32 coord, i32 z);     // 0x16da80 (__thiscall ret 8)
    void Construct(i32 lo, i32 hi); // 0x408710 (shared registry ctor, __thiscall ret 8)
};
struct CActColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};
extern void* GetRetAddr(); // 0x16d990

DATA(0x002bf464)
extern void* g_actCache;
extern void* g_retAddrBreadcrumb;

#endif // GRUNTZ_GRUNTZ_ACTCOLL_H
