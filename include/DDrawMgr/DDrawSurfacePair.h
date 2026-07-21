#ifndef GRUNTZ_CDDRAWSURFACEPAIR_H
#define GRUNTZ_CDDRAWSURFACEPAIR_H

#include <rva.h>
#include <Ints.h>
#include <Wap32/WapObj.h> // CWapObj : CObject - the shared grand-base (slots 0..6)

class CDDSurface;       // +0x2c held surface (CFileImageSurface); <DDrawMgr/DDSurface.h>
class CDDrawSurfaceMgr; // +0x0c parent manager (surface pool at +0x1c)
struct CParseSource;    // LoadImage arg (the 0x139xxx byte-reader)

// (CSurfacePairBase DELETED: dead scaffolding - nothing derived it and nothing
// constructed it; CDDrawSurfacePair derives CWapObj directly and owns the fields.)

SIZE(CDDrawSurfacePair, 0x34); // new-size from CDDrawSubMgrPages::CreateChildren
VTBL(CDDrawSurfacePair, 0x001eff30);
class CDDrawSurfacePair : public CWapObj {
public:
    virtual i32 IsLoaded() OVERRIDE; // slot 5 (@0x14) 0x159090 - the "surface ready?" predicate
    // slot 6 IsReady INHERITED from CWapObj (its `return 1` default @0xd5da0); not
    // redeclared (that was a phantom own-decl under the old flat `: CObject` model).
    // The debug counter-draw pair (bodies in DDrawSurfacePair.cpp, retail birth-
    // positioned dead-center in this class's .text block; ex the fake
    // ResLoaders::DrawHost_164380/DrawHost2_164420 views - their +0x2c "counter
    // window" IS this class's m_surface CDDSurface, whose +0x08 is the DC-capable
    // IDirectDrawSurface). Print n / text centred into rc via GetDC/ReleaseDC.
    void DrawCount(RECT* rc, i32 n);      // 0x00164380
    void DrawLabel(RECT* rc, char* text); // 0x00164420

    i32 m_status;            // +0x04  status word (-1 inactive, 0x63 active)
    i32 m_flags;             // +0x08  create flags (& 0x10000 = make-and-add path)
    CDDrawSurfaceMgr* m_mgr; // +0x0c  parent manager (its surface pool at +0x1c)
public:
    // The spawned-child ctor: the CreateChildren path reuses the shared base-family
    // arg-ctor (??0CLoadable @0x156cb0 - <Gruntz/Loadable.h>, the ex "??0CDDrawSubMgr" -
    // which stamps the CLoadable base vtable 0x5efc30) then manually re-stamps this
    // class's own vtable (g_ddrawSurfacePairVtbl @0x5eff30). Declared-only here (the
    // body IS the shared 0x156cb0 base ctor - the reloc pairs masked; folding this
    // decl away requires the `: CLoadable` family rebase, which renames slots 7/8).
    CDDrawSurfacePair(i32 mgr, i32 kind, i32 a3); // 0x156cb0 (shared base ctor)

    // --- own vtable slots 7..14 (declared-only where the body lives elsewhere) ---
    virtual void TeardownSurface();                        // slot 7  (@0x1c) 0x163e20
    virtual void GetClassId();                          // slot 8  (@0x20) 0x1590c0
    virtual i32 SetGeometry(i32 w, i32 h, i32 bpp); // slot 9 (@0x24) 0x158fd0
    virtual i32 SetGeom(i32 w, i32 h, i32 bpp);     // slot 10 (@0x28) 0x164250
    virtual i32 InitFromSurface(CDDSurface* src);   // slot 11 (@0x2c) 0x163db0
    virtual i32 Create(i32 w, i32 h, i32 bpp, i32 a3);     // slot 12 (@0x30) 0x163c90
    virtual i32 LoadImage(CParseSource* src);       // slot 13 (@0x34) 0x163e50
    virtual i32 ResolveImage_163ee0(CParseSource* src);    // slot 14 (@0x38) 0x163ee0

    virtual ~CDDrawSurfacePair() OVERRIDE; // 0x1590f0  slot 1 (scalar-deleting dtor)

    // --- non-virtual helpers (reconstructed in the owner TU) ------------------
    void BltSelf(CDDrawSurfacePair* src); // 0x03a1d0
    i32 RestoreIfLost();                  // 0x163f00  (surface-lost retry twin of Probe)
    void DrawBox(i32* rect, i32 color);   // 0x163f40
    void DrawCross(i32 x, i32 y);         // 0x164180
    // 0x1644a0 - the DirectDraw mode-surface creator: cache {w,h,bpp}, ask the pool
    // to create the device surface (mode 0x11 if w>320 else 0x51; fullscreen bit
    // from mgr->m_capsFlags), then attach + validate it; on any failure stash a
    // 0x80e9..ed / 0xbb9 / 0xbba error in mgr->m_lastError.
    i32 directx_wrapper_caller_1644a0_DirectDrawCreate_DirectDrawEnumerateA(i32 w, i32 h, i32 bpp);
    // 0x164650 - empty dirty-rect blit hook (retail `ret 0xc` no-op): the
    // CWwdGameObjectC blit dispatch (Slot34/38) calls it per (pos,size) region on
    // the front pair. Reconstructed as an empty body so the 3-byte stub matches.
    void BlitDirtyRect(CDDrawSurfacePair* other, i32* pos, i32* size);
    i32 Probe(); // 0x164660  (surface-lost probe)

    // --- layout (continues the base; base ends at +0x10) ----------------------
    i32 m_width;           // +0x10  width
    i32 m_height;          // +0x14  height
    i32 m_bpp;             // +0x18  bits-per-pixel (8/16/24/32)
    i32 m_srcRect[4];      // +0x1c  x/y offset window {x,y,w,h} (src RECT)
    CDDSurface* m_surface; // +0x2c  the held surface (CFileImageSurface)
    i32 m_ownsSurface;     // +0x30  "owns surface" flag (free on teardown)
};

#endif // GRUNTZ_CDDRAWSURFACEPAIR_H
