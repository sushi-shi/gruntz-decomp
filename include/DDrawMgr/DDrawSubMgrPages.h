#ifndef GRUNTZ_DDRAWMGR_CDDRAWSUBMGRPAGES_H
#define GRUNTZ_DDRAWMGR_CDDRAWSUBMGRPAGES_H

// DDrawSubMgrPages.h - THE single-source shape of CDDrawSubMgrPages, the DDraw
// surface-manager child held at CDDrawSurfaceMgr+0x04 (m_pages). It owns the three
// front/back/overlay surface elements at +0x10/+0x14/+0x18 and back-points at the
// root manager at +0x0c. Retail RTTI ??_7CDDrawSubMgrDraco @0x5efe08 (23 slots).
//
// This class is what USED TO BE modeled as TWO separate views:
//   - CDDrawSubMgrPages (DDrawSubMgrPages.cpp): the page/child factory + teardown
//     (IsLoaded 0x157480, DestroyChildren 0x158ac0, CreateChildren 0x1588f0, the
//     virtual dtor 0x1574d0).
//   - CDDrawWorkerMgr (was include/DDrawMgr/DDrawWorkerMgr.h): the 0x158b40..0x159ef0
//     surface-op method cluster reached by the game as PreviewMgr+0x04 /
//     status-bar +0x04 / FxResource+0x04 / CLevelData+0x04.
// PROVEN the same class: the WorkerMgr `this` uses the SAME +0x10/+0x14/+0x18
// front/back/overlay layout and +0x0c parent back-pointer, and the game holds it at
// CDDrawSurfaceMgr+0x04 (== m_pages). The old WorkerMgr view under-declared a 10-slot
// vtable; the real ??_7 @0x5efe08 carries 23 slots (dump below), so the missing
// 13 slots are declared here.
//
// The +0x0c back-pointer (former WorkerMgr m_worker) is the ROOT CDDrawSurfaceMgr
// (its m_lastError @+0x38, m_pages @+0x04 == this, m_childGroup @+0x08, m_flags
// @+0x34); the "m_worker: CDDrawWorkerNode" naming was a mis-derived view (the
// pointed-to object is the parent, not a standalone worker node) - not migrated.
//
// Polymorphic CWapObj (CObject grand-base): the ctor stamps ??_7CDDrawSubMgrPages
// vptr-first and ~CDDrawSubMgrPages folds the grand-base re-stamp (0x5e8cb4) last, cl-
// emitted - no manual `*(void**)this = &g_*Vtbl` store. The 23-slot own vtable
// @0x5efe08 (slots 0..4 the CObject thunks + the auto-generated scalar-deleting
// dtor ??_G @0x1574b0, slots 5/6 from CWapObj):
//   slot 1  (@0x04)  ??_GCDDrawSubMgrPages  0x1574b0  (cl auto-gen; ~dtor is 0x1574d0)
//   slot 5  (@0x14)  IsLoaded         0x157480  (CWapObj slot-5 override)
//   slot 6  (@0x18)  IsReady          0x001c08  (CWapObj default, inherited)
//   slot 7  (@0x1c)  DestroyChildren  0x158ac0
//   slot 8  (@0x20)  GetStateId       0x1574a0
//   slot 9  (@0x24)  CreateChildren   0x1588f0
//   slot 10 (@0x28)  0x157a20  (a 30-B deleting dtor (??_G) variant)
//   slot 11 (@0x2c)  0x165e30  (COMDAT-folded with CFileMemBase::SetName, filemem)
//   slot 12 (@0x30)  0x157a70
//   slot 13 (@0x34)  0x157a50  (COMDAT-folded with CFileMem::Reset, filemem)
//   slot 14 (@0x38)  0x157920
//   slot 15 (@0x3c)  0x157a00  (Method_159ef0 tail-forwards here - the old Vfunc3c)
//   slot 16 (@0x40)  0x157a10
//   slot 17 (@0x44)  0x157940
//   slot 18 (@0x48)  0x157950
//   slot 19 (@0x4c)  0x165e60  (COMDAT-folded with CFileMem::Open, filemem)
//   slot 20 (@0x50)  0x165ef0  (COMDAT-folded with CFileMem::Ready, filemem)
//   slot 21 (@0x54)  0x165f00  (COMDAT-folded with CFileMem::Read, filemem)
//   slot 22 (@0x58)  0x165f50  (COMDAT-folded with CFileMem::Write, filemem)
// Slots 10..22 are declared-only vNN (bodies live in unmatched/filemem TUs, so no
// RVA() here - they only shape the emitted (reloc-masked) vtable). cl emits
// ??_7CDDrawSubMgrPages only in the owner TU (DDrawSubMgrPages.cpp, where the
// slot-1/5/7/8/9 bodies live); the emitted datum is reloc-masked -> matching-neutral.
//
// Field names are placeholders; only OFFSETS + code bytes are load-bearing.

#include <rva.h>
#include <Ints.h>
#include <Gruntz/StateId.h> // StateId (GetStateId return type)
#include <Wap32/WapObj.h>   // CWapObj : CObject - the real 7-slot grand-base

class CDDrawSurfaceMgr;  // +0x0c root manager back-pointer
class CDDSurface;        // the held surface (CDDrawSurfaceChildA::m_surface)
class CDDrawSurfacePair; // +0x10/+0x14/+0x18 front/back/overlay surface elements

// ---------------------------------------------------------------------------
// CDDrawSubMgrPages (retail RTTI ??_7CDDrawSubMgrDraco @0x5efe08): a 10-slot
// vtable (0x28 B) - byte-proven 2026-07-14: the adjacent 0x1efe30 is the REAL
// ??_7CFileMem (15 separate ctor-stamp code refs), NOT a Pages continuation, so
// the former "slots 10..22" decls here were FABRICATED overhang transcriptions
// of the filemem tables (e.g. the old Slot0F_157a00 "+0x3c slot" is really
// ??_7CFileMem+0x14; 0x159ef0 belongs to CDDrawChildGroup, whose 17-slot table
// makes its `jmp [eax+0x3c]` in-bounds). Slot 5 IsLoaded is an own body
// (0x157480); slot 6 IsReady holds the shared family default 0x1c08 - the
// CWapObj-scheme slot (WapObj.h). Modeled `: CObject` with slots 5/6 declared,
// the family flat-model convention (the vtable_hierarchy audit diffs this
// family against CObject because the abstract CWapObj emits no vtable to diff
// against; the family-wide `: CLoadable` rebase is the flagged intermediate pass).
// ---------------------------------------------------------------------------
SIZE(CDDrawSubMgrPages, 0x1c);
class CDDrawSubMgrPages : public CObject {
public:
    virtual ~CDDrawSubMgrPages() OVERRIDE; // slot 1 (real dtor 0x1574d0)
    // The `??_G` scalar-deleting destructor (slot 1 @0x1574b0): run the real
    // ~CDDrawSubMgrPages (direct call), conditionally RezFree, return this. Hand-written
    // non-virtual + RVA pin (the CFileImageSurface::ScalarDelete pattern) so the body emits.
    void* ScalarDtor(u32 flags);    // 0x1574b0
    virtual i32 IsLoaded();         // slot 5 (@0x14) 0x157480 ("all children present?")
    virtual i32 IsReady();          // slot 6 (@0x18) the shared family default 0x001c08
    virtual void DestroyChildren(); // slot 7 (@0x1c) 0x158ac0
    RVA(0x001574a0, 0x6)
    virtual StateId GetStateId() {
        return STATE_SUBMGRPAGES; // 0xf
    }
    virtual i32 CreateChildren(i32 a1, i32 a2, i32 a3, i32 a4); // slot 9 (@0x24) 0x1588f0

    // --- the 0x158xxx surface-op cluster (was CDDrawWorkerMgr::Method_*) ---------
    i32 Method_158b10(i32 arg1, i32 arg2);     // 0x158b10
    i32 Method_158b40(i32 arg1, i32 arg2);     // 0x158b40
    void Method_158b90();                      // 0x158b90
    i32 Method_158bc0();                       // 0x158bc0
    i32 Method_158bf0(i32 a1, i32 a2, i32 a3); // 0x158bf0
    i32 Method_158cb0(i32 a1, i32 a2);         // 0x158cb0
    void Method_158d50(i32 a1);                // 0x158d50
    i32 Method_158c70(CDDrawSurfacePair* dst); // 0x158c70
    i32 Method_158d20();                       // 0x158d20
    i32 Method_158dc0();                       // 0x158dc0
    i32 Method_158e40();                       // 0x158e40
    i32 Method_158e90();                       // 0x158e90
    i32 Method_158ee0();                       // 0x158ee0

    // vptr @+0x00 (grand-base); the three-word header at +0x04..+0x0c.
    i32 m_04;                         // +0x04  (reset to -1 on teardown)
    i32 m_08;                         // +0x08  (reset to 0)
    CDDrawSurfaceMgr* m_0c;           // +0x0c  root manager back-pointer (reset to 0)
    CDDrawSurfacePair* m_frontPair;   // +0x10  front (Flip target; the "child A" element)
    CDDrawSurfacePair* m_backPair;    // +0x14  back (Fill/geometry source)
    CDDrawSurfacePair* m_overlayPair; // +0x18  overlay (composite)
};
VTBL(CDDrawSubMgrPages, 0x001efe08); // ??_7CDDrawSubMgrPages@@6B@ (10-slot CWapObj-derived vtable)

// The "A" child built by CreateChildren (0x30 bytes, ctor 0x158f30, vtable
// 0x5eff70). Real CWapObj-derived: slots 0..4 inherited, slot 5 the IsLoaded
// override (0x159150, G obj); own slots 7..10 named from their retail slot RVAs
// (SetGeom_1646b0 lives in the T obj). Hoisted from DDrawSubMgrPages.cpp
// (wave4-L). IDENTITY: the ctor 0x158f30 is shared with the CDrawSubWorker view
// (DDrawSubMgr.cpp) - one retail class, two method-set views (identity pass TODO).
class CDDrawSurfaceChildA : public CWapObj {
public:
    virtual ~CDDrawSurfaceChildA() OVERRIDE; // slot 1 (dtor 0x159190)
    virtual i32 IsLoaded() OVERRIDE;         // slot 5 (@0x14) 0x159150 (G obj)
    virtual i32 IsReady() OVERRIDE;          // slot 6 (@0x18) 0x001c08 (CWapObj default)
    virtual void Slot07_1591d0();            // slot 7 (@0x1c) 0x1591d0
    virtual void Slot08_159180();            // slot 8 (@0x20) 0x159180
    virtual i32 CreateModeSurface_1644a0(i32 a1, i32 a2, i32 a3); // slot 9 (@0x24) 0x1644a0
    virtual i32 SetGeom_1646b0(i32 w, i32 h, i32 bpp); // slot 10 (@0x28) 0x1646b0 (T obj)
    CDDrawSurfaceChildA(i32 handle, i32 a2, i32 a3);   // 0x158f30
    i32 m_status;                                      // +0x04  status word (-1 inactive)
    char m_pad08[0x0c - 0x08];
    CDDrawSurfaceMgr* m_mgr; // +0x0c  parent surface manager (its pool at +0x1c)
    i32 m_width;             // +0x10
    i32 m_height;            // +0x14
    i32 m_bpp;               // +0x18
    i32 m_srcRect[4];        // +0x1c  {x,y,w,h}
    CDDSurface* m_surface;   // +0x2c  held surface
}; // 0x30
SIZE(CDDrawSurfaceChildA, 0x30);
VTBL(CDDrawSurfaceChildA, 0x001eff70); // ??_7CDDrawSurfaceChildA@@6B@ (11-slot CWapObj-derived)

#endif // GRUNTZ_DDRAWMGR_CDDRAWSUBMGRPAGES_H
