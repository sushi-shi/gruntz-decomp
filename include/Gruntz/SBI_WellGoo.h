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

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; only the touched members are
// load-bearing; every call through them is reloc-masked).

// The render context reached via drawable->m_14: the object RenderFrame draws
// through (passed by value as its `a` arg) that also holds the DirectDraw
// back-buffer surface at +0x2c (the BltEx `this`). Retail keeps this pointer in a
// callee-saved reg across the whole Tick and reuses it for both RenderFrames + the
// BltEx receiver (ctx->m_2c), so the shared local below re-derives nothing.
struct CGooRenderCtx {
    char m_pad0[0x2c];
    CDDSurface* m_2c; // +0x2c  back-buffer surface (BltEx `this`)
};
SIZE_UNKNOWN(CGooRenderCtx);

// The active drawable reached via g_gameReg->m_30->m_4: its +0x14 is the render
// context (RenderFrame arg + the back-buffer holder above).
struct CGooDrawable {
    char m_pad0[0x14];
    CGooRenderCtx* m_14; // +0x14  render context
};
SIZE_UNKNOWN(CGooDrawable);
struct CGooGameMgr {
    char m_pad0[0x4];
    CGooDrawable* m_4; // +0x04  active drawable
};
SIZE_UNKNOWN(CGooGameMgr);
struct CGooGameReg {
    char m_pad0[0x30];
    CGooGameMgr* m_30; // +0x30  active game manager
};
SIZE_UNKNOWN(CGooGameReg);

// ---------------------------------------------------------------------------
// CSBI_WellGoo - the well-goo status-bar item. Real RTTI base is CSBI_Image (see
// top comment); kept FLAT (frameless method-view) because Tick reads base-region
// storage (m_fillBase/m_fillTop/m_countdown) under goo-specific names that
// CStatusBarItem models as the m_rect14 aggregate - deriving it would erase those
// recovered semantics.
class CSBI_WellGoo {
public:
    // vtable slot 5 (0xe6380): the per-frame goo Tick.
    i32 Tick();

    // ----- layout (offsets are the load-bearing fact) -----
    char m_pad0[0x18];
    i32 m_fillBase; // +0x18  goo fill base (low water line)
    char m_pad1c[0x20 - 0x1c];
    i32 m_fillTop; // +0x20  goo fill top (current water line; Render origin)
    char m_pad24[0x28 - 0x24];
    i32 m_countdown; // +0x28  countdown (Tick decrements; <=0 => idle)
    char m_pad2c[0x34 - 0x2c];
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
    i32 m_fgTop;     // +0x60  ftol(m_fillTop - clampedFill); foreground y top (m_fgTop - 2)
};
SIZE_UNKNOWN(CSBI_WellGoo);

#endif // SBI_WELLGOO_H
