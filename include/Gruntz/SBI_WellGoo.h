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

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; only the touched members/methods are
// load-bearing; every call through them is reloc-masked).

// The per-frame draw handle (the m_40/m_3c anim records): RenderFrame (0x153790,
// __thiscall) blits the frame at a screen position through the surface context.
// Same shape the rest of the SBI image chain uses.
struct CGooFrame {
    void RenderFrame(i32 surfaceCtx, i32 x, i32 y, i32 z); // 0x153790
};

// The active drawable reached via g_gameReg->m_30->m_4: its +0x14 dword is the
// surface context (RenderFrame arg); its +0x2c is the back-buffer surface the
// shade-blit's BltEx draws into.
struct CGooDrawable {
    char m_pad0[0x14];
    i32 m_14; // +0x14  surface context (RenderFrame arg)
    char m_pad18[0x2c - 0x18];
    void* m_2c; // +0x2c  back-buffer surface (BltEx `this`)
};
struct CGooGameMgr {
    char m_pad0[0x4];
    CGooDrawable* m_4; // +0x04  active drawable
};
struct CGooGameReg {
    char m_pad0[0x30];
    CGooGameMgr* m_30; // +0x30  active game manager
};

// The CDDraw shaded-sprite blitter (CSBI_WellGoo::m_38): Blit (0x1497f0,
// __thiscall) validates the dest rect and dispatches a per-mode RLE blit.
struct CGooShadeBlit {
    i32 Blit(i32 p0, void* src, void* clip, i32 sel, i32 p4); // 0x1497f0
};

// The DirectDraw back-buffer surface reached via the drawable's m_2c: BltEx
// (0x13eef0, __thiscall) blits the goo source through a dest/src rect pair.
struct CGooSurface {
    i32 BltEx(void* dstRect, void* src, void* srcRect, u32 flags, void* fx); // 0x13eef0
};

// ---------------------------------------------------------------------------
// CSBI_WellGoo - the well-goo status-bar item.
class CSBI_WellGoo {
public:
    // vtable slot 5 (0xe6380): the per-frame goo Tick.
    i32 Tick();

    // ----- layout (placeholders; offsets are the load-bearing fact) -----
    char m_pad0[0x18];
    i32 m_18; // +0x18  goo fill base (low water line)
    char m_pad1c[0x20 - 0x1c];
    i32 m_20; // +0x20  goo fill top (current water line; Render origin)
    char m_pad24[0x28 - 0x24];
    i32 m_28; // +0x28  countdown (Tick decrements; <=0 => idle)
    char m_pad2c[0x34 - 0x2c];
    void* m_34;      // +0x34  goo source surface/sprite (Blit + BltEx `src`)
    void* m_38;      // +0x38  CGooShadeBlit (Blit `this`)
    CGooFrame* m_3c; // +0x3c  foreground anim frame (final RenderFrame `this`)
    CGooFrame* m_40; // +0x40  base anim frame (first RenderFrame `this`)
    i32 m_44;        // +0x44  fill scale factor (int, fimul); 0 => skip fill
    i32 m_48;        // +0x48  draw x origin
    i32 m_4c;        // +0x4c  src-rect base (lea &m_4c: Blit p0/clip, BltEx srcRect)
    char m_pad50[0x54 - 0x50];
    i32 m_54; // +0x54  draw-depth guard counter (inc around BltEx, dec after)
    i32 m_58; // +0x58  draw-depth guard counter (inc around BltEx, dec after)
    i32 m_5c; // +0x5c  dest-rect base (lea &m_5c: BltEx dstRect)
    i32 m_60; // +0x60  ftol(m_20 - clampedFill); foreground y top (m_60 - 2)
};

#endif // SBI_WELLGOO_H
