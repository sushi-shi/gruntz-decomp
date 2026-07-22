#ifndef GRUNTZ_GRUNTZ_CLIGHTFXRENDER_H
#define GRUNTZ_GRUNTZ_CLIGHTFXRENDER_H

#include <rva.h>

#include <Win32.h> // DirectDraw / Win32 raster types (pure-engine TU, no MFC)

// The light-fx rect IS a plain Win32 RECT (same 4 LONG fields; the ex hand-rolled
// twin forced reinterprets at every MFC-side caller).

class CGruntzMgr;        // the game-manager singleton (Init's arg / m_mgr)
class CTriggerMgr;       // mgr->m_cmdGrid: the 4x15 grunt board (cells = CGrunt)
class CGruntzMapMgr;     // mgr->m_tileGrid: the tile grid (CMapMgr row table)
class CDDrawSurfaceMgr;  // mgr->m_world: the world/resource holder (+0x1c pool)
class CDDSurface;        // the alloc'd DirectDraw work surface (m_surface)
class CDDrawSurfacePair; // the border-draw ctx (its +0x2c CDDSurface is drawn on)

class CLightFxRender {
public:
    // 0x0a32c0  Init - cache the manager family, validate, zero the rect/state block.
    i32 Init(CGruntzMgr* mgr, i32 arg2);
    // 0x0a3360  ctor - zero the core pointers + sizes.
    void Ctor();
    // 0x0a33a0  FreeSurface - release the alloc'd surface (+0x10) via the world pool.
    void FreeSurface();
    // 0x0a33e0  AllocSurface - create the work surface via the world pool.
    i32 AllocSurface();
    // 0x0a3460  (755B) the rebuild/repaint path: (re-)alloc the work surface to the
    // tile grid's dims, lock it, repaint every cell (buffer copy or live-grunt
    // color), unlock. `delta`/`rebuild` gate the partial-decay fast path.
    i32 Resize(i32 delta, i32 rebuild);
    // 0x0a3820  (398B) compute the centered effect rect from a source rect +
    // the chosen scale (m_44), blit the work surface to it, then draw the border
    // framing the live world rect through `ctx`.
    i32 ComputeRect(CDDrawSurfacePair* ctx, RECT* src);
    // 0x0a3a20  DrawBorderRaw - fill the 4 rect edges of `r` with a 16-bit color
    // directly into an already-locked buffer `base`, on this->m_surface's geometry
    // (m_pitch per row, m_b0 per column). No lock/unlock (the caller holds them).
    void DrawBorderRaw(RECT* r, void* base, i32 color);
    // 0x0a3b50  DrawBorder - lock the ctx pair's surface, fill the 4 rect edges
    // with a 16-bit color, unlock. `this`/ecx is unused; ctx supplies the surface.
    void DrawBorder(RECT* r, CDDrawSurfacePair* ctx, i32 color);
    // 0x0a3c90  BuildShape - zero the buffer, dispatch the shape generator.
    i32 BuildShape(i32 shape);
    // The 8 shape generators the switch dispatches to (all in LightFxRender.cpp).
    i32 Shape1(); // 0x0a3dc0
    i32 Shape2(); // 0x0a4890
    i32 Shape3(); // 0x0a5310
    i32 Shape4(); // 0x0a5d90
    i32 Shape5(); // 0x0a67d0
    i32 Shape6(); // 0x0a7260
    i32 Shape7(); // 0x0a7d50
    i32 Shape8(); // 0x0a8900
    // 0x0a4840  FillSpan - fill one horizontal 16-bit span in the +0x4c buffer.
    void FillSpan(u32 x1, u32 x2, u16 color);
    // 0x0a9480  ApplyA - clamp (x,y) to a tile cell, draw via the play state.
    i32 ApplyA(i32 dummy, i32 x, i32 y);
    // 0x0a9500  ClearHandle - drop the +0x48 cached handle.
    i32 ClearHandle(i32 a, i32 b, i32 c);
    // 0x0a9550  ApplyGlobal - clamp + blit through the g_gameReg trigger grid.
    i32 ApplyGlobal(i32 dummy, i32 x, i32 y);
    // 0x0a95d0  ApplyB - like ApplyA but gated on the +0x48 handle.
    i32 ApplyB(i32 dummy, i32 x, i32 y);
    // 0x0a9660  ClampRect - bounds-check + snap (x,y), emit tile-cell out.
    i32 ClampRect(i32 x, i32 y, i32* out, i32 margin);

    // ----- layout (member names mirror the CGruntzMgr slots they cache) -----
    CGruntzMgr* m_mgr;         // +0x00 the game-manager singleton (set by Init)
    CTriggerMgr* m_cmdGrid;    // +0x04 = mgr->m_cmdGrid (+0x68 grunt board)
    CGruntzMapMgr* m_tileGrid; // +0x08 = mgr->m_tileGrid (+0x70 tile grid)
    CDDrawSurfaceMgr* m_world; // +0x0c = mgr->m_world (+0x30 world holder)
    CDDSurface* m_surface;     // +0x10 the alloc'd work surface
    char m_pad14[0x10];
    i32 m_srcL;             // +0x24 source rect L
    i32 m_srcT;             // +0x28 source rect T
    i32 m_srcR;             // +0x2c source rect R
    i32 m_srcB;             // +0x30 source rect B
    i32 m_dstL;             // +0x34 dest/screen rect L
    i32 m_dstT;             // +0x38 dest/screen rect T
    i32 m_dstR;             // +0x3c dest/screen rect R
    i32 m_dstB;             // +0x40 dest/screen rect B
    i32 m_scale;            // +0x44 scale level (0..3) / valid flag
    i32 m_handle;           // +0x48 cached handle (ApplyA latch / ApplyB gate)
    u16 m_buf[0x1f4];       // +0x4c .. +0x433  the 16-bit pixel buffer (500 words)
    i32 m_refreshInterval;  // +0x434 decay-refresh reset value (Resize)
    i32 m_refreshRemaining; // +0x438 decay-refresh countdown
    i32 m_43c;              // +0x43c (role unproven)
};
SIZE_UNKNOWN();

#endif
