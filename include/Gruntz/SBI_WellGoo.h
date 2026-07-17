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
    CDDSurface* m_backBuffer; // +0x2c  back-buffer surface (BltEx `this`)
};
SIZE_UNKNOWN(CGooRenderCtx);

// The active drawable reached via g_gameReg->m_30->m_4: its +0x14 is the render
// context (RenderFrame arg + the back-buffer holder above).
struct CGooDrawable {
    char m_pad0[0x14];
    CGooRenderCtx* m_renderCtx; // +0x14  render context
};
SIZE_UNKNOWN(CGooDrawable);

// Serialize (0xe64c0) callee facets on the same game-manager chain: m_30->m_10 the
// name->frame-set registry (Serialize's Lookup + AnyValueMatches reverse-lookup),
// m_30->m_1c the surface pool (mode-8 MakeAndAddB), m_74 the sprite-ref table
// (mode-8 GetSel). (These extend the goo view of the canonical CGameRegistry; the
// fold onto it is deferred.)
class CDDrawWorkerRegistry; // <DDrawMgr/DDrawWorkerRegistry.h>
class CDDrawPtrCollections; // <DDrawMgr/DDrawPtrCollections.h>
class CSpriteRefTable;      // <Gruntz/SpriteRefTable.h>

// The name->frame-set map value: a frame-pointer array (+0x14) bounded by the
// inclusive frame-index range [m_64, m_68] the read path indexes into.
SIZE_UNKNOWN(CSbiFrameSet);
struct CSbiFrameSet {
    char m_pad0[0x14];
    CImage** m_frames; // +0x14  frame pointer array (indexed by the serialized frame index)
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  low frame index
    i32 m_68; // +0x68  high frame index
};

struct CGooGameMgr {
    char m_pad0[0x4];
    CGooDrawable* m_drawable; // +0x04  active drawable
    char m_pad8[0x10 - 0x8];
    CDDrawWorkerRegistry* m_frameSetRegistry; // +0x10  name->frame-set registry (Serialize)
    char m_pad14[0x1c - 0x14];
    CDDrawPtrCollections* m_surfacePool; // +0x1c  surface pool (Serialize mode-8 MakeAndAddB)
};
SIZE_UNKNOWN(CGooGameMgr);
struct CGooGameReg {
    char m_pad0[0x30];
    CGooGameMgr* m_gameMgr; // +0x30  active game manager
    char m_pad34[0x74 - 0x34];
    CSpriteRefTable* m_refTable; // +0x74  sprite/animation ref table (Serialize mode-8 GetSel)
    // +0x158  the g_focusedGruntSentinel-keyed selector table (mode-8; raw offset access)
};
SIZE_UNKNOWN(CGooGameReg);

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
        m_8 = 7;
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
    virtual i32 Setup(i32 a1, i32 a2, i32 a3, i32 a4, SbiRect rc, i32 a9, i32 a10)
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
