// Loadable.h - the ONE canonical WAP "loadable" intermediate base, unifying the
// three formerly-duplicated CLoadable models (GameLevel.h, CDDrawWorker.h,
// CDDrawWorkerRegistry.cpp) and the fabricated CDDrawSubMgrBase.
//
// Ground truth (retail vtable ??_7CLoadable @0x5efc30 / RVA 0x1efc30, 9 slots):
//   [0] 0x1bef01   CObject slot 0 thunk            }
//   [1] 0x155720   scalar-deleting dtor (??_G)     } from CObject (grand-base 0x5e8cb4)
//   [2] 0x0028ec   CObject slot 2 thunk            }
//   [3] 0x00106e   CObject slot 3 thunk            }
//   [4] 0x004034   CObject slot 4 thunk            }
//   [5] 0x155700   IsLoaded  (CLoadable's own default; overrides CWapObj's 0xd5dc0)
//   [6] 0x001c08   IsReady   (== CWapObj default `return 1`)
//   [7] 0x155740   Unload / reset-hook (new in CLoadable)
//   [8] 0x154a00   GetClassId default: `xor eax,eax; ret` == return CLASSID_NONE (0)
//
// CLoadable : public CWapObj : public CObject. It owns the three-word header
// (m_04/m_08/m_0c) reset by its dtor, and adds the two new virtuals (Unload @[7],
// GetClassId @[8]) directly after CWapObj's IsLoaded/IsReady. ABSTRACT - never
// instantiated on its own, so its intermediate vptr stamps are DEAD-STORED by
// MSVC5 /O2 (no ??_7CLoadable/??_7CWapObj emitted in a leaf's ctor/dtor unless a
// member-ctor barrier keeps one alive; the surviving store reloc-masks against the
// retail 0x5efc30 / grand-base 0x5e8cb4 stamp).
//
// SLOT-1 CONVENTION (why the (B) leaves stay off this base - MECHANISM): this
// canonical uses the (A) real-`virtual ~Dtor()` form via CObject, so cl auto-
// generates each leaf's ??_G scalar-deleting dtor. The (B) leaves instead hand-write
// an explicit `scalar-dtor(i32) OVERRIDE` on a REGULAR-virtual slot-1 to pin ??_G by RVA:
//   CGameLevel          ??_G @0x1611c0  (its own local (B) CLoadable, GameLevel.h)
//   CDDrawSubMgrPages   ??_G @0x1574b0  (: CDDrawSubMgrPagesBase; the retail
//                       ??_7CDDrawSubMgrDraco @0x5efe08 - CDDrawSubMgrDraco IS Pages,
//                       proven by the vtable dump; the ex-"CDDrawSubMgr" dtor 0x1574d0
//                       is ~CDDrawSubMgrPages after the split)
//   CDDrawSubMgrLeaf    ??_G @0x1577c0  (: CDDrawSubMgrGrandBase)
// These CANNOT derive from this (A) base without losing the pinned ??_G, for two
// hard C++/tooling reasons: (1) a real virtual-dtor slot-1 cannot be OVERRIDDEN by a
// non-dtor `scalar-dtor` method, so the leaf can't place its explicit ??_G at slot 1;
// (2) switching them to (A) makes cl AUTO-generate the ??_G, which rva.h cannot RVA-pin
// (it binds a symbol only through a source function def / @llvm.global.annotations
// carrier) -> the currently-100% ??_G drops out of the matched set. So the (B) leaves
// keep a local (B)-form grand-base (regular-virtual slot-1 scalar-dtor:
// CDDrawSubMgrGrandBase / CDDrawSubMgrPagesBase / GameLevel's CLoadable). Full
// unification is a final-sweep item that must flip the ENTIRE CLoadable family (incl.
// matcher-6's (A) leaves) to the (B) explicit-scalar-dtor form.
//
// Field names are placeholders; only offsets + emitted bytes are load-bearing.
#ifndef GRUNTZ_CLOADABLE_H
#define GRUNTZ_CLOADABLE_H

#include <rva.h>
#include <Ints.h>
#include <Wap32/WapObj.h> // CWapObj : CObject - slots 0..6 (IsLoaded/IsReady)

// GetClassId (slot 8) return values are class-type tags. Named here so every
// GetClassId body (base default + each override) returns the named constant, not a
// bare literal (matching-neutral: a named enumerator compiles to the same immediate).
enum LoadableClassId {
    CLASSID_NONE = 0,         // base default @0x154a00 (xor eax,eax; ret)
    CLASSID_IMAGE = 10,       // CImage::GetClassId @0xd5de0 (mov eax,0xa)
    CLASSID_WORKER = 14,      // CDDrawWorker::GetClassId @0x155770 (mov eax,0xe)
    CLASSID_GAMELEVEL = 0x19, // CGameLevel::GetClassId @0x1611b0 (mov eax,0x19)
    // The wide game-object kinds (CWwdGameObject* family; slot-8 bodies are
    // `mov eax,<id>; ret` at the cited RVAs). The serialize-map probes compare
    // GetClassId()==5 - id 5's owning class is not yet recovered (no enumerator).
    CLASSID_WWDOBJ_C = 6,    // CWwdGameObjectC::GetClassId @0x15c020
    CLASSID_WWDOBJ_F = 0x16, // CWwdGameObjectF::GetClassId @0x15ba60
    CLASSID_WWDOBJ_B = 0x1b, // CWwdGameObjectB::GetClassId @0x15bce0
    CLASSID_WWDOBJ_A = 0x1c, // CWwdGameObjectA::GetClassId @0x15b760 (the CreateSprite kind)
};

SIZE(CLoadable, 0x10);
// The 0x1efc30 vtable is CLoadable's OWN (9 slots, no RTTI COL). The former
// "CDDrawSubMgr" identity (include/DDrawMgr/DDrawSubMgr.h + the DDrawSubMgr.cpp-local
// CDDrawSubMgrBase/CDDrawSubMgr pair) was the SAME class under a second name - proven
// by the ctor 0x156cb0 (out-of-line def below in DDrawSubMgr.cpp) stamping this very
// vtable, and slot-for-slot vtable equality ([5] 0x155700 IsLoaded / [6] 0x001c08
// IsReady / [7] 0x155740 Unload / [8] 0x154a00 GetClassId). Folded 2026-07-14; the
// name CDDrawSubMgr is retired.
VTBL(CLoadable, 0x001efc30); // ??_7CLoadable (the shared 9-slot loadable-base vtable)
class CLoadable : public CWapObj {
public:
    // slot 5 IsLoaded: CLoadable's own default @0x155700 (distinct from CWapObj's
    // 0xd5dc0). Declared-only (reloc-masked). Every concrete leaf overrides it.
    virtual i32 IsLoaded() OVERRIDE; // [5] @+0x14  0x155700
    // slot 6 IsReady carries the CWapObj default body (@0x001c08 `return 1`);
    // redeclared per the family convention (the audit diffs src-only tables vs
    // CObject - the abstract CWapObj emits no vtable to diff against).
    virtual i32 IsReady() OVERRIDE; // [6] @+0x18  0x001c08
    virtual i32 Unload();           // [7] @+0x1c  0x155740 (reset/unload hook)
    virtual i32 GetClassId();       // [8] @+0x20  0x154a00 -> CLASSID_NONE

    i32 m_04; // +0x04  (reset to -1 on teardown)
    i32 m_08; // +0x08  (reset to 0)
    i32 m_0c; // +0x0c  (reset to 0)

    CLoadable() {}
    // Arg-taking base ctor - OUT-OF-LINE at 0x156cb0 (DDrawSubMgr.cpp; the ex
    // "??0CDDrawSubMgr"). `owner` is the owning/parent context stored at m_0c
    // (IsLoaded checks it nonzero); field04/field08 are the two managed header
    // words. Retail call sites: CDDrawSurfaceMgr::Init 0x155900, CDDrawSubMgrPages::
    // CreateChildren 0x1588f0, CWwdObjMgr::CreateObject 0x1598d0, CDDrawWorkerHost::
    // ReadPlaneObjects 0x162af0. NB retail ALSO shows the store triple fused into
    // ctors like CResolveNode(i32,i32,i32) @0x15b2c0 - those leaves spell the three
    // stores in their own ctor body (over the default base ctor), reproducing the
    // fused shape without a second definition.
    CLoadable(i32 owner, i32 field04, i32 field08);
    // Field-reset base-subobject dtor: resets the three header fields; the grand-
    // base 0x5e8cb4 re-stamp folds in automatically via ~CWapObj -> ~CObject
    // (no manual `*(void**)this = &g_*Vtbl`).
    virtual ~CLoadable() OVERRIDE {
        m_04 = -1;
        m_08 = 0;
        m_0c = 0;
    }
};

#endif // GRUNTZ_CLOADABLE_H
