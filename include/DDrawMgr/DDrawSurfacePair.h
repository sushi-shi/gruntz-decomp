#ifndef GRUNTZ_CDDRAWSURFACEPAIR_H
#define GRUNTZ_CDDRAWSURFACEPAIR_H

#include <rva.h>
#include <Ints.h>
#include <DDrawMgr/DDrawSubMgrPages.h> // CDrawSubWorker : CLoadable - the geometry base

class CDDSurface;       // +0x2c held surface (CFileImageSurface); <DDrawMgr/DDSurface.h>
class CDDrawSurfaceMgr; // +0x0c parent manager (surface pool at +0x1c)
struct CParseSource;    // LoadImage arg (the 0x139xxx byte-reader)

// (CSurfacePairBase DELETED: dead scaffolding. REBASED 2026-07-22 onto
// CDrawSubWorker: the layouts are identical through +0x2f (m_width/m_height/
// m_bpp/m_srcRect/m_surface at the same offsets), both vtables' slot 9 point at
// the SAME SetGeometry body 0x158fd0 (only inheritance can share one body -
// MSVC5 has no ICF), and the twin IsLoaded bodies 0x159090/0x159150 read the
// same inherited header fields under two spellings.)

class CDDrawSurfacePair : public CDrawSubWorker {
public:
    virtual i32 IsLoaded() OVERRIDE; // slot 5 (@0x14) 0x159090 - the "surface ready?" predicate
    // slot 6 IsReady INHERITED (CWapObj's `return 1` default @0xd5da0); not
    // redeclared (that was a phantom own-decl under the old flat `: CObject` model).
    // The debug counter-draw pair (bodies in DDrawSurfacePair.cpp, retail birth-
    // positioned dead-center in this class's .text block; ex the fake
    // ResLoaders::DrawHost_164380/DrawHost2_164420 views - their +0x2c "counter
    // window" IS this class's m_surface CDDSurface, whose +0x08 is the DC-capable
    // IDirectDrawSurface). Print n / text centred into rc via GetDC/ReleaseDC.
    void DrawCount(RECT* rc, i32 n);      // 0x00164380
    void DrawLabel(RECT* rc, char* text); // 0x00164420

    // +0x04 m_04 (INHERITED, CLoadable): the pool-state word (-1 inactive, 1 =
    // acquire-by-format, 2 = attached, 0x63 = active). +0x08 m_flags (INHERITED):
    // create flags (& 0x10000 = make-and-add path). +0x0c (INHERITED): the parent
    // CDDrawSurfaceMgr - read through OwnerMgr().
public:
    // The spawned-child ctor: the CreateChildren path reuses the shared base-family
    // arg-ctor (??0CLoadable @0x156cb0 - <Gruntz/Loadable.h>, the ex "??0CDDrawSubMgr" -
    // which stamps the CLoadable base vtable 0x5efc30) then manually re-stamps this
    // class's own vtable (??_7 @0x5eff30). Declared-only here (the body IS the
    // shared 0x156cb0 base ctor - the reloc pairs masked). The family rebase the
    // old note asked for happened (: CDrawSubWorker, slots 7/8 = Unload/GetClassId).
    CDDrawSurfacePair(i32 mgr, i32 kind, i32 a3); // 0x156cb0 (shared base ctor)

    // --- slots 7..10 override the CLoadable/CDrawSubWorker scheme; 11..14 are new ---
    // slot 7 Unload (@0x1c) 0x163e20 (ex "TeardownSurface"): release the held
    // surface back to the pool / free it per m_ownsSurface.
    virtual void Unload() OVERRIDE;
    virtual i32 GetClassId() OVERRIDE; // slot 8  (@0x20) 0x1590c0
    // slot 9 SetGeometry INHERITED from CDrawSubWorker (the shared 0x158fd0 body).
    virtual i32 SetGeom(i32 w, i32 h, i32 bpp) OVERRIDE; // slot 10 (@0x28) 0x164250
    virtual i32 InitFromSurface(CDDSurface* src);        // slot 11 (@0x2c) 0x163db0
    virtual i32 Create(i32 w, i32 h, i32 bpp, i32 a3);   // slot 12 (@0x30) 0x163c90
    virtual i32 LoadImage(CParseSource* src);            // slot 13 (@0x34) 0x163e50
    virtual i32 ResolveImage_163ee0(CParseSource* src);  // slot 14 (@0x38) 0x163ee0

    virtual ~CDDrawSurfacePair() OVERRIDE; // 0x1590f0  slot 1 (scalar-deleting dtor)

    // --- non-virtual helpers (reconstructed in the owner TU) ------------------
    void BltSelf(CDDrawSurfacePair* src); // 0x03a1d0
    i32 RestoreIfLost();                  // 0x163f00  (surface-lost retry twin of Probe)
    void DrawBox(i32* rect, i32 color);   // 0x163f40
    void DrawCross(i32 x, i32 y);         // 0x164180
    // (The 0x1644a0 DirectDraw mode-surface creator is CDDrawSurfaceChildA's
    //  slot-9 SetGeometry override - see DDrawSubMgrPages.h; the old
    //  "directx_wrapper_caller_1644a0" decl here was a caller-less duplicate name.)
    // 0x164650 - empty dirty-rect blit hook (retail `ret 0xc` no-op): the
    // CWwdGameObjectC blit dispatch (Slot34/38) calls it per (pos,size) region on
    // the front pair. Reconstructed as an empty body so the 3-byte stub matches.
    void BlitDirtyRect(CDDrawSurfacePair* other, i32* pos, i32* size);
    i32 Probe(); // 0x164660  (surface-lost probe)

    // --- layout: m_width/m_height/m_bpp/m_srcRect/m_surface INHERITED from
    // CDrawSubWorker (base ends at +0x30); this class adds one field -----------
    i32 m_ownsSurface; // +0x30  "owns surface" flag (free on teardown)
};
SIZE(0x34); // new-size from CDDrawSubMgrPages::CreateChildren

#endif // GRUNTZ_CDDRAWSURFACEPAIR_H
