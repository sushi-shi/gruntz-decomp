// DDrawWorkerNode.h - the shared CDDrawWorkerList "worker" class hierarchy. A
// worker is a 0x7c-byte polymorphic frame-animation node the CDDrawWorkerList
// factory (CreateWorker24/28/2C/30) allocates, seeds, and dispatches. Two
// concrete subtypes appear: CDDrawWorkerA (vtable 0x1efea0, 12 slots, BYTE frame at
// +0x78) and CDDrawWorkerB (vtable 0x1efed0, 14 slots, int frame at +0x78).
//
// IDENTITY (proven, realized 2026-07-14): CDDrawWorkerBase IS CResolveNode-derived
// (<Gruntz/ResolveNode.h>). The proof that unlocked it:
//   - 0x164790 IS CResolveNode::SetPosition (slot 9 of ??_7CResolveNode @0x1efbc0 -
//     the vtable datum holds 0x164790 at +0x24), and it is `call`ed BOTH from the
//     wide-object family Setup @0x150d60 (on a CGameObject, a CResolveNode) AND
//     from every worker Vfunc* - so a worker holds a CResolveNode base subobject.
//   - the base field block +0x04..+0x64 maps field-for-field onto CResolveNode
//     (the owner ctx handle @+0x0c == CLoadable::m_0c; m_20/m_38/m_5c/m_64 are its
//     dirty-rect/position sentinels); the workers add only +0x68..+0x7b.
//   - the worker slots [5..9] are OVERRIDES of the CResolveNode scheme (IsLoaded/
//     Unload/GetClassId/SetPosition) with ONE shared body each (the same RVAs sit
//     in BOTH the A and B vtables - a base-class definition, since MSVC5 has no ICF).
// The old ctor blocker (retail builds a worker with a SINGLE inline vptr stamp and
// NO out-of-line `call CResolveNode::CResolveNode()` - disasm 0x157150) is unblocked
// by the CResolveNode NO_SEED tag-ctor (inline + empty: the intermediate stamps are
// dead stores under the derived stamp, MSVC5 /O2 elision).
//
// Field names are placeholders; only offsets + code bytes are load-bearing. The
// +0x0c owner sub-manager handle (CResolveNode/CLoadable's i32 m_0c) is the
// CDDrawWorkerCtx the factories seed; SetPosition/Helper_166040 reinterpret it
// (the CLoadable-family int-handle idiom).
#ifndef GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H
#define GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H

#include <Ints.h>
#include <Gruntz/ResolveNode.h> // CResolveNode : CLoadable - the worker base
#include <rva.h>

// +0x0c owner sub-manager (a CLoadable-family node); its +0x24 int primes m_3c
// and its +0x10 named-object map backs Helper_166040. Completed in HelperHost.cpp.
class CDDrawSurfaceMgr; // the +0x0c owner (the ex-CDDrawWorkerCtx view)

// PlaceFrame's frame-source arg IS the canonical CDDrawWorker (m_items CObArray
// m_pData@+0x14, windowed by m_64/m_68) - the ex CDDrawFrameSource view.
class CDDrawWorker; // the frame-source (ex CDDrawFrameSource view)

// The two surface-pair render targets slot 10 (RenderFrame) draws the worker onto.
// Full def in <DDrawMgr/DDrawSurfacePair.h>, pulled by the method-body TUs.
class CDDrawSurfacePair;

// The worker base: CResolveNode supplies the +0x04..+0x64 field block and vtable
// slots [0..9]; this base OVERRIDES the scheme slots with the family-shared bodies
// (declared-only, reloc-masked - one definition each, present in BOTH leaf vtables)
// and adds the +0x68..+0x77 tail. Abstract (never instantiated): its own emitted
// vtable is a dead COMDAT no retail datum corresponds to - no VTBL.
class CDDrawWorkerBase : public CResolveNode {
public:
    virtual i32 IsLoaded() OVERRIDE;   // [5] 0x157200 (family-shared body)
    virtual i32 Unload() OVERRIDE;     // [7] 0x157310 (family-shared body)
    virtual i32 GetClassId() OVERRIDE; // [8] 0x157210 (family-shared body)
    // [9] 0x157080: the worker re-arm override; the Vfunc* bodies call the BASE
    // 0x164790 directly (qualified CResolveNode::SetPosition - retail's rel32).
    virtual i32 SetPosition(i32 x, i32 y) OVERRIDE; // [9] 0x157080 (family-shared body)
    // [10] the per-frame render onto the two surface pairs (A: 0x165fa0 marker
    // plot, B: 0x1660b0 frame-node blit) - the slot PruneWorkers dispatches per
    // element. Declared here so the list dispatches through the base.
    virtual void RenderFrame(CDDrawSurfacePair* a, CDDrawSurfacePair* b);

    // (+0x68..+0x73 is the base node's m_clip tail - no pad needed: the node ends at +0x74)
    i32 m_refCount;           // +0x74  frames-remaining count (Vfunc* re-arm to 2)
    // +0x78 the frame slot - a BASE field (the shared slot bodies read/clear it as a
    // dword: IsLoaded @0x157200 `[ecx+0x78]!=0`, Unload @0x157310 `[ecx+0x78]=0`),
    // overlaid per-kind: A stores its BYTE frame (factory seed `mov byte [eax+0x78],bl`
    // @0x157012), B its int frame/node pointer (dword seed @0x157192).
    union {
        i32 m_78;   // the dword reading (B's frame node; the base predicates)
        char m_78b; // A's byte frame
    };

    CDDrawWorkerBase() {}
    // The worker-seed ctor: CDDrawWorkerList's CreateWorkerA/B* factories all build a
    // worker with this SAME 9-field base seed (m_0c = the ctx handle, the rest
    // constants - the CResolveNode-default seed set with m_04/m_flags zeroed). The
    // derived CDDrawWorkerA/B ctors delegate here and add m_78. cl emits the base
    // seed, then the DERIVED vptr, then m_78 - matching retail's store order + single
    // vptr stamp; see docs/patterns/ctor-vptr-interleave-vs-spelled-out-init.md.
    CDDrawWorkerBase(CDDrawSurfaceMgr* ctx) : CResolveNode(NO_SEED) {
        m_04 = 0;
        m_0c = reinterpret_cast<i32>(ctx); // the CLoadable-family int owner handle
        m_flags = 0;
        m_20 = static_cast<i32>(0x80000000);
        m_38 = -1;
        m_screenX = static_cast<i32>(0x80000000);
        m_clip.left = static_cast<i32>(0x80000000);
        m_3c = 0;
        m_stateFlags = 0;
    }
};
SIZE(CDDrawWorkerBase, 0x7c);

// BYTE-frame worker (12-slot vtable 0x1efea0): slots 0-9 from the base scheme
// (dtor + RenderFrame overridden), Vfunc2C its own new slot 11.
struct CDDrawWorkerA : public CDDrawWorkerBase {
    virtual ~CDDrawWorkerA() OVERRIDE; // slot 1 (compiler ??_G @0x1570b0; ~ @0x1570d0)
    // [10] 0x165fa0: plot the marker pixel (m_78) at (m_5c,m_60) onto both pairs.
    virtual void RenderFrame(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE;
    CDDrawWorkerA() {}
    CDDrawWorkerA(CDDrawSurfaceMgr* ctx) : CDDrawWorkerBase(ctx) {
        m_78b = 0; // the BYTE frame seed (retail `mov byte [eax+0x78],bl`)
    }
    virtual i32 Vfunc2C(i32 a1, i32 a2, i32 a3); // [11] 0x157110
    // +0x78 byte frame = the base union's m_78b.
};
SIZE(CDDrawWorkerA, 0x7c);
VTBL(CDDrawWorkerA, 0x001efea0); // vtable_names -> code (RTTI game class)

// int-frame worker (14-slot vtable 0x1efed0): adds PlaceFrame (slot 12) / PlaceBound
// (slot 13) plus the non-virtual named-object frame fetch Helper_166040 (0x166040).
struct CDDrawWorkerB : public CDDrawWorkerBase {
    virtual ~CDDrawWorkerB() OVERRIDE; // slot 1 (compiler ??_G @0x157220; ~ @0x157240)
    // [10] 0x1660b0: draw the current frame node (m_78) onto the two surface-pair
    // targets (unconditional first, gated second on m_2c live + not flagged 0x20000).
    virtual void RenderFrame(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE;
    CDDrawWorkerB() {}
    CDDrawWorkerB(CDDrawSurfaceMgr* ctx) : CDDrawWorkerBase(ctx) {
        m_78 = 0;
    }
    virtual i32 Vfunc2C(i32 a1, i32 a2, i32 a3);                    // [11] 0x1572f0
    virtual i32 PlaceFrame(i32 a1, i32 a2, CDDrawWorker* src, i32 a4); // [12] 0x1572b0
    virtual i32 PlaceBound(i32 a1, i32 a2, i32 a3, i32 a4);            // [13] 0x157280

    // Non-virtual: look up a named object in the owner map, fetch element[idx] when
    // in range, cache at m_78, return whether it is non-null.
    i32 Helper_166040(i32 key, i32 idx); // 0x166040
    // +0x78 int frame/node = the base union's m_78.
};
SIZE(CDDrawWorkerB, 0x7c);

// ??_7CDDrawWorkerB (was g_ddrawWorkerBVtbl). Emitted by `new CDDrawWorkerB` in
// CDDrawWorkerList.cpp; the slot-11/12/13 relocs now name the real overrides.
VTBL(CDDrawWorkerB, 0x001efed0);

// --- vtable catalog ---

#endif // GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H
