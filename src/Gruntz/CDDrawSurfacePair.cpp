#include <rva.h>
// CDDrawSurfacePair.cpp - a surface-backed drawing region in the DDrawMgr
// "Remus" image family. It owns one held DDraw surface (a CPoolItemA, the
// CDDSurface wrapper) borrowed from the parent CDirectDrawMgr's surface pool,
// plus a cached pixel geometry (width @+0x10 / height @+0x14 / bpp @+0x18) and an
// x/y offset window @+0x1c. Its own vtable is @0x5eff30; the grand-base dtor
// vtable is g_remusBaseDtorVtbl @0x5e8cb4. See include/Gruntz/CDDrawSurfacePair.h.
//
// This TU carries four of the class's methods (in retail-RVA order):
//   BltSelf       (0x03a1d0) - BltFast another pair's surface onto ours
//   ~dtor         (0x1590f0) - teardown chain + base-vtable restore (/GX EH frame)
//   TeardownSurface (0x163e20, vtable slot 7) - remove the surface from the pool
//   DrawBox       (0x163f40) - draw a 1px rectangle outline into the locked surface
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine). The engine callees (CDDSurface
// Lock/BltFast, the pool RemoveItemA, IDirectDrawSurface::Unlock) are reloc-
// masked external __thiscall/__stdcall calls.
// ---------------------------------------------------------------------------

#include <Gruntz/CDDrawSurfacePair.h>
#include <string.h> // memset for the edge-row fills (inline rep-stos CRT)

// ---------------------------------------------------------------------------
// The locked-surface pixel geometry view: byte-pitch @+0x20 and bytes-per-pixel
// @+0xb0 of the held CDDSurface, plus its IDirectDrawSurface @+0x08 (for Unlock).
// A layout-compatible view onto CDDSurface so the pixel math lowers to direct
// [surface+0x20]/[surface+0xb0] loads (the offsets are load-bearing).
// ---------------------------------------------------------------------------
class DrawSurfaceView {
public:
    char m_pad00[0x08 - 0x00]; // +0x00..+0x07
    IDirectDrawSurfaceZ* m_8;  // +0x08  held IDirectDrawSurface (Unlock)
    char m_pad0c[0x20 - 0x0c]; // +0x0c..+0x1f
    i32 m_pitch;               // +0x20  byte pitch (bytes per row)
    char m_pad24[0xb0 - 0x24]; // +0x24..+0xaf
    i32 m_bpp;                 // +0xb0  bytes per pixel
};

// ---------------------------------------------------------------------------
// 0x03a1d0: BltFast `src`'s held surface onto ours at (0,0) with the source's
// offset window (&src->m_rect1c) as the src rect and DDBLTFAST_SRCCOLORKEY|WAIT
// (0x10) flags.  __thiscall, one ptr arg.
RVA(0x0003a1d0, 0x1d)
void CDDrawSurfacePair::BltSelf(CDDrawSurfacePair* src) {
    m_2c->BltFast(0, 0, src->m_2c, &src->m_rect1c, 0x10);
}

// ---------------------------------------------------------------------------
// 0x1590f0: the (non-deleting) destructor. Stamps this class's own vtable, runs
// the teardown (remove the surface from the pool), zeroes the width, then the
// base subobject dtor folds in (zeroes m_08/m_0c, sets m_04=-1, stamps the
// grand-base dtor vtable). /GX EH frame from the non-trivial base subobject.
// @early-stop
// 91.56% - every instruction byte matches retail; the only residual is the EH
// scope-table pointer (retail `push 0x8`/Unwind@005e1e40 vs our `push 0x0`/$L800
// funclet) + reloc-masked vtable symbol names (g_surfacePairVtbl, the grand-base
// dtor vtbl). Documented EH-table-offset wall; objdiff-reloc-scoring.
RVA(0x001590f0, 0x56)
CDDrawSurfacePair::~CDDrawSurfacePair() {
    *(void**)this = &g_surfacePairVtbl;
    TeardownSurface();
    m_10 = 0;
    // ~CSurfacePairBase() folds here: m_08=0; m_0c=0; m_04=-1; stamp grand-base.
}

// ---------------------------------------------------------------------------
// 0x163e20: the surface teardown (vtable slot 7). When the held surface is
// present AND owned (m_30 set), remove it from the parent manager's surface pool;
// then zero m_2c. Always zero the width (m_10).
RVA(0x00163e20, 0x2d)
void CDDrawSurfacePair::TeardownSurface() {
    if (m_2c != 0 && m_30 != 0) {
        CDDrawSurfacePool* pool = *(CDDrawSurfacePool**)((char*)m_0c + 0x1c);
        pool->RemoveItemA(m_2c);
        m_2c = 0;
    }
    m_10 = 0;
}

// ---------------------------------------------------------------------------
// 0x163f40: draw a 1-pixel rectangle outline of `color` along the rect
// {left,top,right,bottom} (all four inside [0,width)x[0,height)). Locks the held
// surface, fills the top + bottom edge rows then walks the left + right edge
// columns, then unlocks. 8bpp (1 byte/pixel) and 16bpp (2 bytes/pixel) paths.
// @early-stop
// 52.74% - big function (574 B, 200 vs 196 instrs): logic/CFG/offsets/calls all
// reproduced (the bounds checks, Lock, the 8/16-bpp byte-replication rep-stos row
// fills, the per-row left/right column writes, Unlock). Residual is a regalloc /
// stack-frame wall: retail keeps `this` spilled + `rect` in esi (sub esp,0x8)
// while our build keeps `this` in esi + `rect` in ecx (sub esp,0x18), cascading
// different [esp+N] slot choices through the body. Deferred to the final sweep.
RVA(0x00163f40, 0x23e)
void CDDrawSurfacePair::DrawBox(i32* rect, i32 color) {
    i32 left = rect[0];
    if (left < 0 || left >= m_10) {
        return;
    }
    i32 top = rect[1];
    if (top < 0 || top >= m_14) {
        return;
    }
    i32 right = rect[2];
    if (right < 0 || right >= m_10) {
        return;
    }
    i32 bottom = rect[3];
    if (bottom < 0 || bottom >= m_14) {
        return;
    }
    char* base = (char*)m_2c->Lock(0);
    if (base == 0) {
        return;
    }
    DrawSurfaceView* sv = (DrawSurfaceView*)m_2c;
    u8 c = (u8)color;
    i32 w = right - left + 1;

    // ---- top + bottom horizontal edges ----
    if (m_18 == 0x10) {
        i32 n = 2 * w;
        if (n > 0) {
            memset(base + sv->m_bpp * left + sv->m_pitch * top, c, n);
        }
        if (n > 0) {
            memset(base + sv->m_bpp * left + sv->m_pitch * bottom, c, n);
        }
    } else {
        if (w > 0) {
            memset(base + sv->m_bpp * left + sv->m_pitch * top, c, w);
        }
        if (w > 0) {
            memset(base + sv->m_bpp * left + sv->m_pitch * bottom, c, w);
        }
    }

    // ---- left + right vertical edges ----
    i32 h = bottom - top + 1;
    if (h > 0) {
        for (i32 y = 0; y < h; ++y) {
            if (m_18 == 0x10) {
                i32 lo = (top + y) * sv->m_pitch + sv->m_bpp * left;
                base[lo] = c;
                base[lo + 1] = c;
                i32 ro = (top + y) * sv->m_pitch + sv->m_bpp * right;
                base[ro] = c;
                base[ro + 1] = c;
            } else {
                i32 lo = (top + y) * sv->m_pitch + sv->m_bpp * left;
                base[lo] = c;
                i32 ro = (top + y) * sv->m_pitch + sv->m_bpp * right;
                base[ro] = c;
            }
        }
    }

    // Unlock the held surface.
    sv->m_8->vtbl->Unlock(sv->m_8, 0);
}
