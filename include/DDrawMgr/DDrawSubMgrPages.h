#ifndef GRUNTZ_DDRAWMGR_CDDRAWSUBMGRPAGES_H
#define GRUNTZ_DDRAWMGR_CDDRAWSUBMGRPAGES_H

#include <rva.h>
#include <Ints.h>
#include <Gruntz/StateId.h>  // StateId (GetStateId return type)
#include <Wap32/WapObj.h>    // CWapObj : CObject - the real 7-slot grand-base
#include <Gruntz/Loadable.h> // CLoadable (the CDrawSubWorker family base)

class CDDrawSurfaceMgr;  // +0x0c root manager back-pointer
class CDDSurface;        // the held surface (CDDrawSurfaceChildA::m_surface)
class CDDrawSurfacePair; // +0x10/+0x14/+0x18 front/back/overlay surface elements

SIZE(CDDrawSubMgrPages, 0x1c);
class CDDrawSubMgrPages : public CWapObj {
public:
    virtual ~CDDrawSubMgrPages() OVERRIDE; // slot 1 (real dtor 0x1574d0)
    // The `??_G` scalar-deleting destructor (slot 1 @0x1574b0): run the real
    // ~CDDrawSubMgrPages (direct call), conditionally RezFree, return this. Hand-written
    // non-virtual + RVA pin (the CFileImageSurface::ScalarDelete pattern) so the body emits.
    void* ScalarDtor(u32 flags);    // 0x1574b0
    virtual i32 IsLoaded() OVERRIDE; // slot 5 (@0x14) 0x157480 ("all children present?")
    // slot 6 IsReady INHERITED from CWapObj (the shared `return 1` default @0xd5da0,
    // reached via the 0x001c08 thunk); not redeclared (that was a phantom own-decl).
    virtual void DestroyChildren(); // slot 7 (@0x1c) 0x158ac0
    RVA(0x001574a0, 0x6)
    virtual StateId GetStateId() {
        return STATE_SUBMGRPAGES; // 0xf
    }
    virtual i32 CreateChildren(i32 a1, i32 a2, i32 a3, i32 a4); // slot 9 (@0x24) 0x1588f0

    // --- the 0x158xxx surface-op cluster (was CDDrawWorkerMgr::Method_*) ---------
    i32 Method_158b10(struct CParseSource* src, i32 arg2); // 0x158b10 (ResolveImage the page from the parse record)
    i32 Method_158b40(struct CParseSource* src, i32 arg2); // 0x158b40 (LoadImage the page from the parse record)
    void Method_158b90();                      // 0x158b90
    i32 PagesReady();                       // 0x158bc0
    i32 Method_158bf0(i32 a1, i32 a2, i32 a3); // 0x158bf0
    i32 Method_158cb0(i32 a1, i32 a2);         // 0x158cb0
    void Method_158d50(i32 a1);                // 0x158d50
    i32 BlitPage(CDDrawSurfacePair* dst); // 0x158c70
    i32 Method_158d20();                       // 0x158d20
    i32 Method_158dc0();                       // 0x158dc0
    i32 TransEnter();                       // 0x158e40
    i32 TransTitle();                       // 0x158e90
    i32 TransExit();                       // 0x158ee0

    // vptr @+0x00 (grand-base); the three-word header at +0x04..+0x0c.
    i32 m_04;                         // +0x04  (reset to -1 on teardown)
    i32 m_08;                         // +0x08  (reset to 0)
    CDDrawSurfaceMgr* m_0c;           // +0x0c  root manager back-pointer (reset to 0)
    CDDrawSurfacePair* m_frontPair;   // +0x10  front (Flip target; the "child A" element)
    CDDrawSurfacePair* m_backPair;    // +0x14  back (Fill/geometry source)
    CDDrawSurfacePair* m_overlayPair; // +0x18  overlay (composite)
};
VTBL(CDDrawSubMgrPages, 0x001efe08); // ??_7CDDrawSubMgrPages@@6B@ (10-slot CWapObj-derived vtable)

// ---------------------------------------------------------------------------
// CDrawSubWorker - the 0x30 surface-holder BASE (vtable ??_7 @0x5effa0; ctor
// 0x158f30, ??1 0x158fb0 / ??_G 0x158f90 - all in the G obj DDrawSubMgr.cpp): a
// CLoadable leaf caching the {w,h,bpp} pixel geometry + a {0,0,w,h} src rect +
// the held-surface slot (+0x2c). PROVEN base of CDDrawSurfaceChildA (retail
// CreateChildren inlines the derived ctor: `call 0x158f30; mov [edi],0x5eff70;
// mov [edi+0x2c],0`) AND - by the shared slot-9 body 0x158fd0 in BOTH raw
// vtables (0x1eff30[9] == 0x1effa0[9]; MSVC5 has no ICF, so a shared slot IS
// inheritance) - of CDDrawSurfacePair, whose flat model keeps its own slot-9
// claim for now (pair re-base pass TODO; its inline ctor's `call 0x156cb0` +
// m_width=0 is the base ctor per-site re-inlined by /Ob2). The class name is
// scaffolding (@identity-TODO - no RTTI names this family).
// ---------------------------------------------------------------------------
class CDrawSubWorker : public CLoadable {
public:
    // Out-of-line @0x158f30: the CLoadable seed fused into the leaf ctor (the
    // CResolveNode(i32,i32,i32) precedent in Loadable.h) + m_width = 0.
    CDrawSubWorker(i32 a1, i32 a2, i32 a3);
    virtual i32 IsLoaded() OVERRIDE;   // [5] 0x158f60 (declared-only, unreconstructed)
    virtual i32 Unload() OVERRIDE;     // [7] 0x159080 (declared-only, unreconstructed)
    virtual i32 GetClassId() OVERRIDE; // [8] 0x158f80 (declared-only, unreconstructed)
    // [9] 0x158fd0: cache the {w,h,bpp} geometry + src rect. The BODY is bound as
    // CDDrawSurfacePair::SetGeometry_158fd0 until the pair re-bases here (one
    // claim per rva); this base decl + ChildA's override reloc-mask.
    virtual i32 SetGeometry(i32 w, i32 h, i32 bpp); // [9] 0x158fd0
    // [10] 0x159020: SetGeometry with bpp-in-{8,16,24,32} validation (G obj def).
    virtual i32 SetGeom(i32 w, i32 h, i32 bpp); // [10] 0x159020
    // slot-1 dtor: INLINE so the derived dtors fold these very resets (retail
    // ~CDDrawSurfaceChildA @0x1591b0 is m_04/m_width/m_08/m_0c inline, no base
    // call); the linker-kept out-of-line COMDAT copy is the retail ??1 @0x158fb0
    // (bound by @rva-symbol in DDrawSubMgr.cpp).
    virtual ~CDrawSubWorker() OVERRIDE {
        m_width = 0;
    }

    i32 m_width;           // +0x10  (zeroed by ctor + dtor)
    i32 m_height;          // +0x14
    i32 m_bpp;             // +0x18
    i32 m_srcRect[4];      // +0x1c..+0x2b  {0,0,w,h}
    CDDSurface* m_surface; // +0x2c  held surface slot (the derived ctors zero it)
}; // 0x30
SIZE(CDrawSubWorker, 0x30);
VTBL(CDrawSubWorker, 0x001effa0); // ??_7CDrawSubWorker (11-slot CLoadable leaf)

class CDDrawSurfaceChildA : public CDrawSubWorker {
public:
    // Inline ctor - retail CreateChildren shows exactly this expansion:
    // `call 0x158f30` (the out-of-line base ctor) + own ??_7 stamp + m_surface=0.
    CDDrawSurfaceChildA(i32 handle, i32 a2, i32 a3) : CDrawSubWorker(handle, a2, a3) {
        m_surface = 0;
    }
    // slot-1 ??1 @0x1591b0 / ??_G @0x159190 (DDrawSubMgr.cpp). The out-of-line
    // body is EMPTY: retail's four resets are the inlined ~CDrawSubWorker +
    // ~CLoadable.
    virtual ~CDDrawSurfaceChildA() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;   // [5] 0x159150 (G obj def)
    virtual i32 Unload() OVERRIDE;     // [7] 0x1591d0 (declared-only)
    virtual i32 GetClassId() OVERRIDE; // [8] 0x159180 (declared-only)
    // [9] 0x1644a0 (T obj def): create the display-mode surface + set geometry
    // (the ex "CreateModeSurface_1644a0"; it IS the SetGeometry override).
    virtual i32 SetGeometry(i32 a1, i32 a2, i32 a3) OVERRIDE;
    virtual i32 SetGeom(i32 w, i32 h, i32 bpp) OVERRIDE; // [10] 0x1646b0 (T obj def)
}; // 0x30 (no own fields)
SIZE(CDDrawSurfaceChildA, 0x30);
VTBL(CDDrawSurfaceChildA, 0x001eff70); // ??_7CDDrawSurfaceChildA@@6B@ (11 slots)

#endif // GRUNTZ_DDRAWMGR_CDDRAWSUBMGRPAGES_H
