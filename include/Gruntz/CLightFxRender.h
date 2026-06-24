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
    int left;
    int top;
    int right;
    int bottom;
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

class CLightFxRender {
public:
    // 0x0a32c0  Init - copy mgr fields, validate, zero the rect/state block.
    int Init(LfxMgr* mgr, int arg2);
    // 0x0a3360  ctor - zero the core pointers + sizes.
    void Ctor();
    // 0x0a33a0  FreeSurface - release the alloc'd surface (+0x10) via the mgr.
    void FreeSurface();
    // 0x0a33e0  AllocSurface - create the work surface via the surface mgr.
    int AllocSurface();
    // 0x0a3460  (755B) the resize/realloc path - deferred.
    int Resize(int arg1, int arg2);
    // 0x0a3820  (398B) compute the effect rect from a source rect + scale.
    int ComputeRect(LfxRect* src);
    // 0x0a3b50  DrawBorder - lock surface, fill 4 rect edges 16-bit, unlock.
    void DrawBorder(LfxRect* r, unsigned short color);
    // 0x0a3c90  BuildShape - zero the buffer, dispatch the shape generator.
    int BuildShape(int shape);
    // The 8 shape generators the switch dispatches to. Four are in this TU's
    // target set (RVA stubs, deferred to the final sweep); the other four
    // (Shape2/5/7/8) live in adjacent TUs - declared so the calls reloc-mask.
    int Shape1(); // 0x0a3dc0 (deferred)
    int Shape2(); // 0x0a4890 (extern)
    int Shape3(); // 0x0a5310 (deferred)
    int Shape4(); // 0x0a5d90 (deferred)
    int Shape5(); // 0x0a67d0 (extern)
    int Shape6(); // 0x0a7260 (deferred)
    int Shape7(); // 0x0a7d50 (extern)
    int Shape8(); // 0x0a8900 (extern)
    // 0x0a4840  FillSpan - fill one horizontal 16-bit span in the +0x4c buffer.
    void FillSpan(unsigned x1, unsigned x2, unsigned short color);
    // 0x0a9480  ApplyA - clamp (x,y) to a tile cell, draw via the mgr context.
    int ApplyA(int dummy, int x, int y);
    // 0x0a9500  ClearHandle - drop the +0x48 cached handle.
    int ClearHandle(int a, int b, int c);
    // 0x0a9550  ApplyGlobal - clamp + blit through the g_gameReg surface.
    int ApplyGlobal(int dummy, int x, int y);
    // 0x0a95d0  ApplyB - like ApplyA but gated on the +0x48 handle.
    int ApplyB(int dummy, int x, int y);
    // 0x0a9660  ClampRect - bounds-check + snap (x,y), emit tile-cell out.
    int ClampRect(int x, int y, int* out, int margin);

    // ----- layout (placeholders; offsets are load-bearing) -----
    LfxMgr* m_00;     // +0x00 game/render manager (set by Init)
    void* m_04;       // +0x04 mgr+0x68
    void* m_08;       // +0x08 mgr+0x70
    LfxSurfMgr* m_0c; // +0x0c mgr+0x30 (the surface manager)
    LfxSurface* m_10; // +0x10 the alloc'd work surface
    char m_pad14[0x10];
    int m_24;                    // +0x24 source rect L
    int m_28;                    // +0x28 source rect T
    int m_2c;                    // +0x2c source rect R
    int m_30;                    // +0x30 source rect B
    int m_34;                    // +0x34 screen rect L
    int m_38;                    // +0x38 screen rect T
    int m_3c;                    // +0x3c screen rect R
    int m_40;                    // +0x40 screen rect B
    int m_44;                    // +0x44 scale level (0..3) / valid flag
    int m_48;                    // +0x48 cached handle
    unsigned short m_buf[0x1f4]; // +0x4c .. +0x433  the 16-bit pixel buffer (500 words)
    int m_434;                   // +0x434 buffer total
    int m_438;                   // +0x438 remaining
    int m_43c;
};

#endif
