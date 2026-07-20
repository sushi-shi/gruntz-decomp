// SBI_WellGoo.h - Gruntz CSBI_WellGoo (C:\Proj\Gruntz).
// RTTI .?AVCSBI_WellGoo@@; the most-derived leaf of the SBI image chain
//   CSBI_WellGoo : CSBI_Image : CSBI_RectOnly : CStatusBarItem.
// Vtable @0x5eadfc. The 4-level /GX-framed scalar destructor (0x104bb0) lives in
// SBI_WellGooEh.cpp.
//
// This leaf adds, over CSBI_Image, the per-frame Tick (0xe6380, vtable slot 5):
// a countdown-gated animation step that draws the well's goo level - a base anim
// frame, a clamped shade-blit fill whose height tracks (m_20 - m_18) progress, and
// a foreground frame.
//
// Fields are placeholders; the offsets + code bytes are the load-bearing fact (the
// mangled ?<method>@CSBI_WellGoo@@... names are layout-independent). The class is
// modeled with the SBI family's manual-vtable-stamp device (no real `virtual`), so
// the frameless Tick matches without forcing a divergent compiler vtable;
// sibling/engine callees are ILT/vtable-reloc-masked.
#ifndef SBI_WELLGOO_H
#define SBI_WELLGOO_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SBI_Image.h> // CSBI_Image base

// The serialize stream: the REAL CFileMemBase (<Gruntz/SerialArchive.h> typedefs
// CSerialArchive onto it). Pointer-only here, so the fwd decl + typedef suffice;
// an elaborated `struct CSerialArchive*` would re-declare a DISTINCT class and
// silently out-rank the typedef (MSVC5).
class CFileMemBase;
typedef CFileMemBase CSerialArchive;

// ---------------------------------------------------------------------------
// The per-frame draw handles (m_40/m_3c) ARE the RTTI CImage (CImage::RenderFrame
// @0x153790); the owned blitter m_38 IS the CDDrawShadeBlit (CDDrawShadeBlit::Blit
// @0x1497f0); the goo source m_34 + the back-buffer are CDDSurface (CDDSurface::
// BltEx @0x13eef0). Full defs are pulled in the .cpp; only pointer types are needed
// here, so forward-declare the unified classes (was CGooFrame/CGooShadeBlit/
// CGooSurface placeholder views).
class CImage;
class CDDSurface;
class CDDrawShadeBlit;

// (The five "goo view" structs are GONE - every one was a canonical class:
//   CGooGameReg    == CGruntzMgr      (m_gameMgr@+0x30==m_world, m_refTable@+0x74==m_spriteFactory)
//   CGooGameMgr    == CDDrawSurfaceMgr (drawable@+0x04==m_drawTarget, registry@+0x10==m_imageRegistry,
//                                       pool@+0x1c==m_ptrColl)
//   CGooDrawable   == CDDrawSubMgrPages (renderCtx@+0x14==m_backPair)
//   CGooRenderCtx  == CDDrawSurfacePair (backBuffer@+0x2c==m_surface)
//   CSbiFrameSet   == CImageSet (the FIFTH view of the 0x6c CDDrawWorker frame shape)
// and the "+0x158 selector table" was m_options[g_curPlayer].m_008 all along.)
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Image/ImageSet.h> // CImageSet == CDDrawWorker (the ONE frame-set class)
#include <Gruntz/SpriteRefTable.h>

// ---------------------------------------------------------------------------
// CSBI_WellGoo - the well-goo status-bar item. Real RTTI base is CSBI_Image (see
// top comment); kept FLAT (frameless method-view) because Tick reads base-region
// storage (m_fillBase/m_fillTop/m_countdown) under goo-specific names that
// CStatusBarItem models as the m_rect14 aggregate - deriving it would erase those
// recovered semantics.
class CSBI_WellGoo : public CSBI_Image {
public:
    // tag 7 (the Gruntz-tab WELLGOO widget).
    CSBI_WellGoo() {
        m_kind = 7;
        m_30 = 0;
    }
    // Real vtable shape (sema class: vtbl@0x1eadfc, 12 slots; overrides 0/1/2/3/4/5).
    // The out-of-line ~ (0x104bb0) lives in SBI_WellGoo.cpp via the CHAIN-DTOR device
    // (see StatusBarItem.h).
    virtual ~CSBI_WellGoo() OVERRIDE; // slot 0
    // slot 1 (vtbl 0x1eadfc thunk 0x3e90 -> 0xe64c0): the goo serialize. Round-trips the
    // fill/rect fields + two frame handles by name+index, (mode 8) re-resolves the goo
    // surface + rebinds the frames' shade nodes, then chains CSBI_Image::SerializeFields.
    virtual i32 SerializeFields(CSerialArchive* arc, i32 mode, i32 a3, i32 a4) OVERRIDE; // 0xe64c0
    virtual i32 Setup(CStatusBarMgr* owner, CDDrawSurfaceMgr* host, i32 a3, i32 a4, SbiRect rc, i32 a9, i32 a10)
        OVERRIDE; // slot 2 (0xe6020; args 5..8 are ONE by-value SbRect - see StatusBarItem.h)
    virtual void SbiSlot3() OVERRIDE; // slot 3 (the Free below)
    virtual void SbiSlot4() OVERRIDE; // slot 4
    virtual void SbiSlot5() OVERRIDE; // slot 5 (the Tick below)

    // vtable slot 5 (0xe6380): the per-frame goo Tick. Goo state reuses the base
    // region: fillBase=m_rect14.m_4 (@0x18), fillTop=m_rect14.m_c (@0x20), countdown=m_28.
    i32 Tick();

    // (0xe64c0 was declared here as a non-virtual `Serialize` - it IS the slot-1
    //  SerializeFields override declared above. Its base leg 0xe6e40 is CSBI_Image's and
    //  is reached with a QUALIFIED CSBI_Image::SerializeFields call - never re-declared
    //  here, which would emit a CSBI_WellGoo symbol that resolves nowhere.)

    // vtable slot 3 (0x104c80): release the owned goo source surface through the
    // cached manager's (+0x24) surface pool, then clear it.
    void Free();

    // ----- own fields (after CSBI_Image @0x34) -----
    CDDSurface* m_gooSrc;       // +0x34  goo source surface (Blit + BltEx `src`)
    CDDrawShadeBlit* m_blitter; // +0x38  owned shaded-sprite blitter (Blit `this`)
    CImage* m_fgFrame;          // +0x3c  foreground frame record (final RenderFrame `this`)
    CImage* m_baseFrame;        // +0x40  base frame record (first RenderFrame `this`)
    i32 m_fillScale;            // +0x44  fill scale factor (int, fimul); 0 => skip fill
    i32 m_drawX;                // +0x48  draw x origin
    i32 m_srcRect; // +0x4c  src-rect base (lea &m_srcRect: Blit p0/clip, BltEx srcRect)
    char m_pad50[0x54 - 0x50];
    i32 m_drawGuard; // +0x54  draw-depth guard counter (inc around BltEx, dec after)
    i32 m_blitGuard; // +0x58  draw-depth guard counter (inc around BltEx, dec after)
    i32 m_dstRect;   // +0x5c  dest-rect base (lea &m_dstRect: BltEx dstRect)
    i32 m_fgTop;     // +0x60  ftol(fillTop - clampedFill); foreground y top (m_fgTop - 2)
};
SIZE_UNKNOWN(CSBI_WellGoo);
VTBL(CSBI_WellGoo, 0x001eadfc); // vtable_names -> code (RTTI game class)

#endif // SBI_WELLGOO_H
