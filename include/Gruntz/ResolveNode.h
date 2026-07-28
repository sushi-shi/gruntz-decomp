#ifndef GRUNTZ_GRUNTZ_RESOLVENODE_H
#define GRUNTZ_GRUNTZ_RESOLVENODE_H

#include <Gruntz/Loadable.h> // canonical CLoadable : CWapObj : CObject (9-slot base)
#include <Ints.h>
#include <DDrawMgr/ShadeTableCache.h> // CShadeTable - the +0x4c fill table
#include <rva.h>

class CDDrawSurfaceMgr; // fwd (was the dissolved CImageParent pad-view)

// The 0x24-byte dirty-rect record. Every wide object carries TWO of them: the LIVE
// one at CResolveNode +0x18 and the SHADOW (previous-frame) one at CGameObject
// +0xb8 - which is why CGameObject's blit slots copy one onto the other with a
// single 36-byte move and serialize it as one 0x24 blob.
// Ctor 0x15b270 (`{ m_rect.left = INT_MIN; m_armed = -1; }`) is a real out-of-line
// COMDAT: CDDrawChildGroup::CreateObject_159250/159440/159600 CALL it on
// `this+0xb8` between the +0x9c region ctor and the +0xdc CString ctor, and the
// /GX ctor-in-flight state walks 1 -> 2 -> 3 across those three calls.
// CResolveNode's live copy at +0x18 is this SAME type (folded 2026-07-28 from six
// loose fields), which is what lets the C/A blit slots write the snapshot as a
// plain `m_shadow = m_dirty` struct assignment.
struct WwdDirtyRect {
    WwdDirtyRect(); // 0x15b270 (COMDAT copy in WwdObjMgr.cpp)
    // Empty, but USER-DECLARED, and that is byte-evidence: a member with a
    // destructor gets its own /GX unwind-map entry, and retail's ctor-in-flight
    // state walk in CreateObject_159250/159440/159600 spends one index on this
    // record (0 raw / 1 CResolveNode / 2 region / 3 THIS / 4 CString / 5 worker).
    // Without it cl numbers the states 0/1/2/3 and every byte after shifts.
    ~WwdDirtyRect() {}
    // No-seed tag ctor, the mirror of CResolveNode::ENoSeed below and needed for the
    // same reason: the CDDrawWorkerBase family spells its OWN fused seed set in its
    // ctor body, and retail proves the record's ctor did not also run there -
    // CDDrawWorkerList::CreateWorkerA/B28/B2C (@0x157150 ff) emit the +0x20/+0x38
    // pair ONCE, after the m_id/m_ownerCtx/m_flags trio (body position), not twice
    // and not in mem-init position.
    enum ENoSeed {
        NO_SEED
    };
    WwdDirtyRect(ENoSeed) {}
    i32 m_lastX; // +0x00  last drawn column (was CResolveNode's m_lastX)
    i32 m_lastY; // +0x04  last drawn row    (was CResolveNode's m_lastY)
    RECT m_rect; // +0x08  the blit out-rect (.left INT_MIN == disarmed corner)
    i32 m_w;     // +0x18  extent x
    i32 m_h;     // +0x1c  extent y
    i32 m_armed; // +0x20  armed flag (-1 == disarmed; also the blit path's result
                 //        out: 0 ok / -1 culled - the same latch)
};
SIZE(0x24);

// INLINE by default (retail's copy is a /Gy COMDAT that CGameObject::CGameObject
// @0x15b390 folds while the three CDDrawChildGroup factories call it); the guard
// is the per-TU switch - see the block in <Gruntz/WwdGridIter.h>.
//   WWDDIRTYRECT_OOL_CTOR -> WwdObjMgr.cpp (0x15b270)
#ifndef WWDDIRTYRECT_OOL_CTOR
inline WwdDirtyRect::WwdDirtyRect() {
    m_rect.left = static_cast<i32>(0x80000000);
    m_armed = -1;
}
#endif

class CResolveNode : public CLoadable {
public:
    // Re-based onto the canonical 9-slot CLoadable: the m_04/m_08/m_0c header +
    // slots 5..8 come from CLoadable. This node OVERRIDES slot 5 (IsLoaded
    // @0x154a10) and slot 7 (Unload/reset @0x154a80); slots 6/8 carry the
    // CLoadable default bodies (0x001c08 / 0x154a00) and are INHERITED (audit:
    // redeclare-nothing now that CLoadable's own vtable 0x1efc30 is bound).
    // SetPosition (slot 9) is the node's own new virtual.
    virtual i32 IsLoaded() OVERRIDE; // [5] 0x154a10  (checks m_04!=-1 && m_0c)
    virtual void Unload() OVERRIDE;  // [7] 0x154a80  disarm the dirty-rect sentinels
    // slot 9 (new): set position + reset the draw state (x->m_5c, y->m_60, zero
    // the clip/plot fields, reseed m_48=0x32/m_50=1, cache owner->m_24). Body
    // 0x164790 (T obj); shared by the whole wide-object family (never overridden).
    virtual i32 SetPosition(i32 x, i32 y); // [9] 0x164790

    CResolveNode(); // 0x1549d0 (D pocket)
    // INLINE by default, guarded per-TU (see the block below + <Gruntz/WwdGridIter.h>):
    // CreateObject_159250/159440/159600 emit `call 0x15b2c0`, so wwdobjmgr defines
    // CRESOLVENODE_OOL_CTOR; CGameObject::CGameObject @0x15b390 FOLDS the body, so
    // wwdfactoryobject keeps it inline and forces the standalone COMDAT out instead.
    CResolveNode(CDDrawSurfaceMgr* owner, i32 field04, i32 field08);
    // No-seed tag-ctor for the worker leaves (CDDrawWorkerBase family): constructs
    // the base WITHOUT the 0x1549d0 sentinel seeding - the leaf ctor spells its own
    // fused seed set, matching the factories' single-stamp inline shape (retail
    // 0x157150: one derived stamp, no base-ctor call). Inline + empty so the
    // intermediate vptr stamp dies as a dead store under the derived stamp.
    enum ENoSeed {
        NO_SEED
    };
    CResolveNode(ENoSeed) : m_dirty(WwdDirtyRect::NO_SEED) {}
    i32 Init(
        CDDrawSurfaceMgr* owner,
        i32 field04,
        i32 resolveX,
        i32 resolveY,
        i32 field40,
        i32 field08
    );
    // ^ 0x1647e0 (T obj)

    // Dtor: disarm the live dirty-rect sentinels, then ~CLoadable (m_04/-1,
    // m_08/m_0c zero) + the CObject grand-base restamp fold in. Defined
    // OUT-OF-LINE in WwdFactoryObject.cpp (0x154a50) - the SAME TU as the
    // wide-object family dtors, so cl folds its content into ~E/~A/~F/~C
    // exactly as retail does, while the ResolveNode.cpp pocket's ??_G emits
    // retail's `call 0x154a50` against the extern (an inline def here would
    // make cl fold the body INTO that ??_G, which retail did not).
    virtual ~CResolveNode() OVERRIDE; // 0x154a50 (WwdFactoryObject.cpp)

    // vptr @+0x00 + m_04/m_08/m_0c inherited from CLoadable; own fields from +0x10.
    // (Names merged from the wide-object family's proven readers - the ex
    // WwdEdgeA/WwdEdgeB RAII scaffolding and the flat views' +0x10..+0x64 block.)
    i32 m_plotDX; // +0x10  (SetPosition zeroes; plot state)
    i32 m_plotDY; // +0x14  (SetPosition zeroes)
    // +0x18..+0x3b  the LIVE dirty-rect record - the same WwdDirtyRect type the wide
    // objects carry a second (shadow/previous-frame) copy of at CGameObject +0xb8,
    // which is why the C/A blit slots snapshot one onto the other with a single
    // 36-byte move. Folded 2026-07-28 from six loose fields (m_lastX/m_lastY/
    // m_dirtyRect/m_dirtyW/m_dirtyH/m_dirtyArmed - names migrated onto the type):
    // BYTE-PROVEN by the ctors, where the seed pair (+0x20 .left = INT_MIN, +0x38
    // armed = -1) is emitted as ONE unit BEFORE the ??_7CResolveNode stamp
    // (0x15b2c0 / the inlined copy in 0x15b390), i.e. as a MEMBER ctor in the
    // mem-init half, not as two body stores after the vptr.
    WwdDirtyRect m_dirty;
    // +0x3c  the owning level (SetPosition seeds OwnerMgr()->m_level; the blit path
    // hops m_level->m_mainPlane->WrapCoord). Was the i32 "m_3c" + the ex-CBlitXform
    // view's +0x5c plane hop - CBlitXform WAS CGameLevel.
    class CGameLevel* m_level;
    i32 m_stateFlags;           // +0x40  (SetPosition zeroes)
    i32 m_44;                   // +0x44  (SetPosition zeroes)
    i32 m_48;                   // +0x48  (SetPosition reseeds 0x32)
    CShadeTable* m_drawFillArg; // +0x4c  the fill shade table (SetPosition zeroes)
    i32 m_drawFillCmd;          // +0x50  (SetPosition reseeds 1; 0xb = decay fill-bar)
    i32 m_fillFraction;         // +0x54  fill fraction (0..256)
    i32 m_drawActive;           // +0x58  dirty/active flag (SetPosition zeroes)
    i32 m_screenX;              // +0x5c  screen/position X (INT_MIN = unset; the flat
                                //        CGameObject model's m_screenX - name converged)
    i32 m_screenY;              // +0x60  screen/position Y
    // +0x64..+0x73  the record clip rect - ONE member, wholly the node's: LevelTile-
    // Validation passes it BY VALUE (a RECT straddling a base boundary cannot exist),
    // the D-ctor seeds only .left (the INT_MIN sentinel), and the worker leaves'
    // +0x68..+0x73 is an untouched pad with their own m_refCount at +0x74 - so the
    // node ends at +0x74, not +0x68. (.left/.top/.right are also the checkpoint
    // config triple, slots 12..14.)
    RECT m_clip; // +0x64
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();
VTBL(CResolveNode, 0x001efbc0); // ??_7CResolveNode@@6B@ (10 slots; ex WwdBResolve dup)

// INLINE by default - the same /Gy COMDAT arrangement as the two node ctors in
// <Gruntz/WwdGridIter.h>: CGameObject::CGameObject @0x15b390 FOLDS this body (its
// +0x20/+0x38/+0x5c/+0x64 sentinel stores and the 0x5efbc0 stamp are spelled inline
// there, no call), while the three CDDrawChildGroup factories call the 0x15b2c0 copy.
//   CRESOLVENODE_OOL_CTOR -> WwdObjMgr.cpp (its factories CALL it). The standalone
//   0x15b2c0 copy is forced out of WwdFactoryObject.cpp, which needs it inline.
#ifndef CRESOLVENODE_OOL_CTOR
inline CResolveNode::CResolveNode(CDDrawSurfaceMgr* owner, i32 field04, i32 field08)
    : CLoadable(field04, field08, owner) {
    // (the +0x20 .left / +0x38 armed seed pair is m_dirty's OWN default ctor now -
    // that is why retail emits it before the vptr stamp, not after)
    m_screenX = static_cast<i32>(0x80000000);
    m_clip.left = static_cast<i32>(0x80000000);
    m_level = 0;
    m_stateFlags = 0;
}
#endif

#endif // GRUNTZ_GRUNTZ_RESOLVENODE_H
