// Loadable.h - the ONE canonical WAP "loadable" intermediate base, unifying the
// three formerly-duplicated CLoadable models (GameLevel.h, CDDrawWorker.h,
// CDDrawWorkerRegistry.cpp) and the fabricated CDDrawSubMgrBase.
//
// Ground truth (retail vtable ??_7CLoadable @0x5efc30 / RVA 0x1efc30, 9 slots):
//   [0] 0x1bef01   CObject slot 0 thunk            }
//   [1] 0x155720   scalar-deleting dtor (??_G)     } from Wap::CObject (grand-base 0x5e8cb4)
//   [2] 0x0028ec   CObject slot 2 thunk            }
//   [3] 0x00106e   CObject slot 3 thunk            }
//   [4] 0x004034   CObject slot 4 thunk            }
//   [5] 0x155700   IsLoaded  (CLoadable's own default; overrides CWapObj's 0xd5dc0)
//   [6] 0x001c08   IsReady   (== CWapObj default `return 1`)
//   [7] 0x155740   Unload / reset-hook (new in CLoadable)
//   [8] 0x154a00   GetClassId default: `xor eax,eax; ret` == return CLASSID_NONE (0)
//
// CLoadable : public CWapObj : public Wap::CObject. It owns the three-word header
// (m_04/m_08/m_0c) reset by its dtor, and adds the two new virtuals (Unload @[7],
// GetClassId @[8]) directly after CWapObj's IsLoaded/IsReady. ABSTRACT - never
// instantiated on its own, so its intermediate vptr stamps are DEAD-STORED by
// MSVC5 /O2 (no ??_7CLoadable/??_7CWapObj emitted in a leaf's ctor/dtor unless a
// member-ctor barrier keeps one alive; the surviving store reloc-masks against the
// retail 0x5efc30 / grand-base 0x5e8cb4 stamp).
//
// SLOT-1 CONVENTION (why the (B) leaves stay off this base - MECHANISM): this
// canonical uses the (A) real-`virtual ~Dtor()` form via Wap::CObject, so cl auto-
// generates each leaf's ??_G scalar-deleting dtor. The (B) leaves instead hand-write
// an explicit `ScalarDtor(i32) OVERRIDE` on a REGULAR-virtual slot-1 to pin ??_G by RVA:
//   CGameLevel          ??_G @0x1611c0  (its own local (B) CLoadable, GameLevel.h)
//   CDDrawSubMgrPages   ??_G @0x1574b0  (: CDDrawSubMgrPagesBase; the retail
//                       ??_7CDDrawSubMgrDraco @0x5efe08 - CDDrawSubMgrDraco IS Pages,
//                       proven by the vtable dump; the ex-"CDDrawSubMgr" dtor 0x1574d0
//                       is ~CDDrawSubMgrPages after the split)
//   CDDrawSubMgrLeaf    ??_G @0x1577c0  (: CDDrawSubMgrGrandBase)
// These CANNOT derive from this (A) base without losing the pinned ??_G, for two
// hard C++/tooling reasons: (1) a real virtual-dtor slot-1 cannot be OVERRIDDEN by a
// non-dtor `ScalarDtor` method, so the leaf can't place its explicit ??_G at slot 1;
// (2) switching them to (A) makes cl AUTO-generate the ??_G, which rva.h cannot RVA-pin
// (it binds a symbol only through a source function def / @llvm.global.annotations
// carrier) -> the currently-100% ??_G drops out of the matched set. So the (B) leaves
// keep a local (B)-form grand-base (regular-virtual slot-1 ScalarDtor:
// CDDrawSubMgrGrandBase / CDDrawSubMgrPagesBase / GameLevel's CLoadable). Full
// unification is a final-sweep item that must flip the ENTIRE CLoadable family (incl.
// matcher-6's (A) leaves) to the (B) explicit-ScalarDtor form.
//
// Field names are placeholders; only offsets + emitted bytes are load-bearing.
#ifndef GRUNTZ_CLOADABLE_H
#define GRUNTZ_CLOADABLE_H

#include <rva.h>
#include <Ints.h>
#include <Wap32/WapObj.h> // CWapObj : Wap::CObject - slots 0..6 (IsLoaded/IsReady)

// GetClassId (slot 8) return values are class-type tags. Named here so every
// GetClassId body (base default + each override) returns the named constant, not a
// bare literal (matching-neutral: a named enumerator compiles to the same immediate).
enum LoadableClassId {
    CLASSID_NONE = 0,         // base default @0x154a00 (xor eax,eax; ret)
    CLASSID_IMAGE = 10,       // CImage::GetClassId @0xd5de0 (mov eax,0xa)
    CLASSID_WORKER = 14,      // CDDrawWorker::GetClassId @0x155770 (mov eax,0xe)
    CLASSID_GAMELEVEL = 0x19, // CGameLevel::GetClassId @0x1611b0 (mov eax,0x19)
};

SIZE(CLoadable, 0x10);
class CLoadable : public CWapObj {
public:
    // slot 5 IsLoaded: CLoadable's own default @0x155700 (distinct from CWapObj's
    // 0xd5dc0). Declared-only (reloc-masked). Every concrete leaf overrides it.
    i32 IsLoaded(); // [5] @+0x14  0x155700
    // slots 6 IsReady is inherited from CWapObj (default @0x001c08 `return 1`).
    i32 Unload();     // [7] @+0x1c  0x155740 (reset/unload hook)
    i32 GetClassId(); // [8] @+0x20  0x154a00 -> CLASSID_NONE

    i32 m_04; // +0x04  (reset to -1 on teardown)
    i32 m_08; // +0x08  (reset to 0)
    i32 m_0c; // +0x0c  (reset to 0)

    CLoadable() {}
    // Arg-taking base ctor: cl inlines the vptr stamp + the three field stores at
    // the head of a derived ctor. `owner` is the owning/parent context stored at
    // m_0c (IsLoaded checks it nonzero); field04/field08 are the two managed header
    // words. Params are in derived-ctor arg order (owner first) so a leaf forwards
    // them straight through; the store order (m_04, m_08, m_0c) is the retail one.
    CLoadable(i32 owner, i32 field04, i32 field08) {
        m_04 = field04;
        m_08 = field08;
        m_0c = owner;
    }
    // Field-reset base-subobject dtor: resets the three header fields; the grand-
    // base 0x5e8cb4 re-stamp folds in automatically via ~CWapObj -> ~Wap::CObject
    // (no manual `*(void**)this = &g_*Vtbl`).
    ~CLoadable() {
        m_04 = -1;
        m_08 = 0;
        m_0c = 0;
    }
};

#endif // GRUNTZ_CLOADABLE_H
