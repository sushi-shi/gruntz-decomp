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
// external/no-body so the calls reloc-mask; g_projActCache (0x6bf464) is the real
// shared scratch buffer Insert consumes (the retail-canonical name for 0x6bf464;
// the former per-file g_actCache alias was UNBOUND - 0x6bf464 binds to g_projActCache).
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

DATA(0x002bf464)
extern void* g_projActCache; // 0x6bf464 (?g_projActCache@@3PAXA - canonical bound name)
// Legacy per-file alias for the SAME 0x6bf464 scratch buffer; kept declared so the
// TUs that still spell it (Projectile/DroppedObject/GruntVoice/FortressFlag) compile.
// 0x6bf464 binds to g_projActCache (dedup keep-last winner), so g_actCache is UNBOUND
// - references reached through the shared inlines below use g_projActCache instead.
extern void* g_actCache;
extern void* g_retAddrBreadcrumb;

#endif // GRUNTZ_GRUNTZ_ACTCOLL_H
