// CLightFxRender.h - a non-polymorphic (no vftable / no RTTI) software light /
// glow effect renderer in the lighting-effects module (between CLightFx @0x49cf00
// and CDoNothingNormal @0x4a9e00). The object (~0x440 bytes) owns an embedded
// 16-bit pixel buffer at +0x4c (~500 words) that the shape generators fill, a
// source/screen RECT pair at +0x24..+0x40, and pointers to the surface / lighting
// managers. It allocates a DirectDraw surface via the manager's vtable, fills the
// buffer with computed 16-bit (565/555) colors using the screen RGB shift globals
// (0x683ea0..0x683eb4), and blits.
//
// Recovered from a tracer placeholder (ClassUnknown_68). Non-polymorphic: NONE of
// the method addresses appear as data anywhere in the EXE (no vtable), so every
// method is a plain __thiscall. Field names are placeholders (m_<hexoffset>); only
// the OFFSETS + code bytes are load-bearing (campaign doctrine). Layout recovered
// from the ctor/init field stores + the field reads in the draw/clip paths.
#ifndef GRUNTZ_GRUNTZ_CLIGHTFXRENDER_H
#define GRUNTZ_GRUNTZ_CLIGHTFXRENDER_H

#include <rva.h>

#include <Win32.h> // DirectDraw / Win32 raster types (pure-engine TU, no MFC)

// ---------------------------------------------------------------------------
// A 4-int RECT (left, top, right, bottom) - the engine's plain integer rect.
struct LfxRect {
    i32 left;
    i32 top;
    i32 right;
    i32 bottom;
};

// The surface manager the renderer talks to (the object at this+0xc / this+0x10).
// Only the offsets the reconstructed paths touch are modeled.
//   +0x18 / +0x1c : tile/zoom pixel dims (idiv divisors)
//   +0x1c         : surface-alloc / -free dispatch (a vtable-like fn table)
struct LfxSurfMgr;

// The render manager (this+0x00, set by Init); +0x2c holds the draw context.
struct LfxMgr;

// The allocated draw surface (this+0x2c held by callers; +0x10 held by us).
//   +0x20  : bytes-per-pixel / x-stride
//   +0xb0  : surface pitch (bytes per scanline)
struct LfxSurface;

// The global game-manager singleton (the object at *0x64556c, ?g_gameReg). Only
// the +0x68 slot the blit path reads is modeled.
struct CGameReg;

// The border-draw context (DrawBorder's 2nd arg): +0x2c is the locked DirectDraw
// work surface (pitch/stride/Lock), +0x08 the unlock interface (vtable[0x20]).
struct LfxBorderCtx;

class CLightFxRender {
public:
    // 0x0a32c0  Init - copy mgr fields, validate, zero the rect/state block.
    i32 Init(LfxMgr* mgr, i32 arg2);
    // 0x0a3360  ctor - zero the core pointers + sizes.
    void Ctor();
    // 0x0a33a0  FreeSurface - release the alloc'd surface (+0x10) via the mgr.
    void FreeSurface();
    // 0x0a33e0  AllocSurface - create the work surface via the surface mgr.
    i32 AllocSurface();
    // 0x0a3460  (755B) the resize/realloc path - deferred.
    i32 Resize(i32 arg1, i32 arg2);
    // 0x0a3820  (398B) compute the centered effect rect from a source rect +
    // the chosen scale (m_44), alloc the work surface, then draw the border.
    i32 ComputeRect(LfxRect* src);
    // 0x0a3b50  DrawBorder - lock the ctx surface, fill the 4 rect edges with a
    // 16-bit color, unlock. `this`/ecx is unused; ctx supplies the surface.
    void DrawBorder(LfxRect* r, LfxBorderCtx* ctx, i32 color);
    // 0x0a3c90  BuildShape - zero the buffer, dispatch the shape generator.
    i32 BuildShape(i32 shape);
    // The 8 shape generators the switch dispatches to. Four are in this TU's
    // target set (RVA stubs, deferred to the final sweep); the other four
    // (Shape2/5/7/8) live in adjacent TUs - declared so the calls reloc-mask.
    i32 Shape1(); // 0x0a3dc0 (deferred)
    i32 Shape2(); // 0x0a4890 (extern)
    i32 Shape3(); // 0x0a5310 (deferred)
    i32 Shape4(); // 0x0a5d90 (deferred)
    i32 Shape5(); // 0x0a67d0 (extern)
    i32 Shape6(); // 0x0a7260 (deferred)
    i32 Shape7(); // 0x0a7d50 (extern)
    i32 Shape8(); // 0x0a8900 (extern)
    // 0x0a4840  FillSpan - fill one horizontal 16-bit span in the +0x4c buffer.
    void FillSpan(u32 x1, u32 x2, u16 color);
    // 0x0a9480  ApplyA - clamp (x,y) to a tile cell, draw via the mgr context.
    i32 ApplyA(i32 dummy, i32 x, i32 y);
    // 0x0a9500  ClearHandle - drop the +0x48 cached handle.
    i32 ClearHandle(i32 a, i32 b, i32 c);
    // 0x0a9550  ApplyGlobal - clamp + blit through the g_gameReg surface.
    i32 ApplyGlobal(i32 dummy, i32 x, i32 y);
    // 0x0a95d0  ApplyB - like ApplyA but gated on the +0x48 handle.
    i32 ApplyB(i32 dummy, i32 x, i32 y);
    // 0x0a9660  ClampRect - bounds-check + snap (x,y), emit tile-cell out.
    i32 ClampRect(i32 x, i32 y, i32* out, i32 margin);

    // ----- layout (placeholders; offsets are load-bearing) -----
    LfxMgr* m_00;     // +0x00 game/render manager (set by Init)
    void* m_04;       // +0x04 mgr+0x68
    void* m_08;       // +0x08 mgr+0x70
    LfxSurfMgr* m_0c; // +0x0c mgr+0x30 (the surface manager)
    LfxSurface* m_10; // +0x10 the alloc'd work surface
    char m_pad14[0x10];
    i32 m_24;         // +0x24 source rect L
    i32 m_28;         // +0x28 source rect T
    i32 m_2c;         // +0x2c source rect R
    i32 m_30;         // +0x30 source rect B
    i32 m_34;         // +0x34 screen rect L
    i32 m_38;         // +0x38 screen rect T
    i32 m_3c;         // +0x3c screen rect R
    i32 m_40;         // +0x40 screen rect B
    i32 m_44;         // +0x44 scale level (0..3) / valid flag
    i32 m_48;         // +0x48 cached handle
    u16 m_buf[0x1f4]; // +0x4c .. +0x433  the 16-bit pixel buffer (500 words)
    i32 m_434;        // +0x434 buffer total
    i32 m_438;        // +0x438 remaining
    i32 m_43c;
};

#endif
