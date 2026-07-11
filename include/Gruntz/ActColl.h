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
// external/no-body so the calls reloc-mask. The shared scratch buffer Insert consumes
// is bound canonically as g_projActCache @0x2bf464 (in GruntStartingPoint.cpp); the
// old g_actCache "0x6bf464" was a VA-typo alias of the SAME global that lost the DATA
// dedup - the shared inlines below reference the bound g_projActCache, so their DATA
// relocs are faithful. g_actCache stays declared only for direct .cpp users not yet
// unified (KitchenSlime/DroppedObject/ToobSpikez/StaticHazard/SecretTeleporter/... ).
#ifndef GRUNTZ_GRUNTZ_ACTCOLL_H
#define GRUNTZ_GRUNTZ_ACTCOLL_H

#include <rva.h>
#include <Wap32/ZVec.h>           // real _zvec (Find @0x16da80 = _zvec::GrowTo)
#include <Wap32/ZDArrayDerived.h> // real CZDArrayDerived (Construct/RegisterRange @0x8710)

// The coordinate collection itself: its first dword is the collection object (a
// zDArray-family vtable/base). Every registry IS-A CActColl at +0x00 (CActReg
// derives from it), so the slow Find lookup is a direct base call - no view cast.
// CActColl's 3 methods are all views of real fns (cast at each call, no local methods):
//   Find @0x16da80 = _zvec::GrowTo,  Construct/RegisterRange @0x8710 = CZDArrayDerived::Construct.
struct CActColl {
    void* m_coll; // +0x00  the collection object (its first dword)
};
extern void* GetRetAddr(); // 0x16d990

// The shared act-node alloc-scratch cache. g_projActCache is the canonical bound name
// (@0x2bf464, GruntStartingPoint.cpp); g_actCache is a same-global alias kept declared
// (unbound) for the .cpp bodies that still spell it that way.
extern void* g_projActCache; // 0x2bf464 (?g_projActCache@@3PAXA)
extern void* g_actCache;     // alias of g_projActCache (unbound; use g_projActCache)
extern void* g_retAddrBreadcrumb;

#endif // GRUNTZ_GRUNTZ_ACTCOLL_H
