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
// 0x163c90: Create(w, h, bpp, flags) - cache the {w,h,bpp} geometry + the
// {0,0,w,h} source rect, then acquire a held surface from the parent manager's
// pool: when m_04 == 1 via AcquireA(pixel-format token, 4); otherwise via
// MakeAndAddB/CreateB (selected by the 0x10000 flag bit). On any failure record a
// last-error code (0xfa1/0xfa2/0xfa3/0xfa4) in the manager (only if still 0) and
// return 0; on success flag the surface as owned (m_30=1) and return 1. The
// two m_04 paths are TWO sequential ifs (if m_04==1 {...} if m_04!=1 {...}), which
// is why the success merge re-tests m_04. __thiscall, 4 stack args (ret 0x10).
// @early-stop
// 96.02% - logic/CFG/offsets/calls/error-codes all reproduced. The lone residual is
// a 1-instruction regalloc coin-flip in the w<=0 error path: retail loads m_04 into
// eax (mov eax,[esi+4]; cmp $1,eax) so it can reuse esi for the manager, we emit the
// shorter direct compare (cmp $1,[esi+4]). Same values, same branch. Not source-
// steerable; docs/patterns/zero-register-pinning.md family.
RVA(0x00163c90, 0x116)
i32 CDDrawSurfacePair::Create(i32 w, i32 h, i32 bpp, i32 a3) {
    m_08 = a3;
    if (w <= 0 || h <= 0) {
        i32 k = m_04;
        CDDrawSurfaceMgr* mgr = (CDDrawSurfaceMgr*)m_0c;
        if (k == 1) {
            if (mgr->m_38 == 0) {
                mgr->m_38 = 0xfa1;
            }
        } else {
            if (mgr->m_38 == 0) {
                mgr->m_38 = 0xfa2;
            }
        }
        return 0;
    }
    m_10 = w;
    m_14 = h;
    m_18 = bpp;
    i32* rect = (i32*)m_rect1c;
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = w;
    rect[3] = h;
    if (m_04 == 1) {
        CDDrawSurfaceMgr* mgr = (CDDrawSurfaceMgr*)m_0c;
        m_2c = mgr->m_1c->AcquireA(mgr->m_4->m_10->m_2c, 4);
        if (m_2c == 0) {
            if (((CDDrawSurfaceMgr*)m_0c)->m_38 == 0) {
                ((CDDrawSurfaceMgr*)m_0c)->m_38 = 0xfa3;
            }
            return 0;
        }
    }
    if (m_04 != 1) {
        if (m_08 & 0x10000) {
            m_2c = ((CDDrawSurfaceMgr*)m_0c)->m_1c->MakeAndAddB(w, h, 0, 0, -1);
        } else {
            m_2c = ((CDDrawSurfaceMgr*)m_0c)->m_1c->CreateB(w, h, 0, 0, -1);
        }
        if (m_2c == 0) {
            if (((CDDrawSurfaceMgr*)m_0c)->m_38 == 0) {
                ((CDDrawSurfaceMgr*)m_0c)->m_38 = 0xfa4;
            }
            return 0;
        }
    }
    m_30 = 1;
    return 1;
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
// 0x163f00: restore the held surface if it was lost. With no surface, report OK
// (1). Otherwise, when the held IDirectDrawSurface is present and not lost
// (IsLost, slot 24 @+0x60, returns DD_OK), report OK; else Restore it (slot 27
// @+0x6c) and report whether the restore succeeded. __thiscall, no args.
// @early-stop
// regalloc coin-flip (98.67%): every code byte matches retail (incl. the setcc
// boolean) EXCEPT the register the m_2c->m_8 re-read lands in (retail eax / ours
// edx) + the carried scratch reg in the setcc tail (ecx vs edx). Same values, same
// stores; not source-steerable. docs/patterns/zero-register-pinning.md.
RVA(0x00163f00, 0x40)
i32 CDDrawSurfacePair::RestoreIfLost() {
    if (m_2c == 0) {
        return 1;
    }
    IDirectDrawSurfaceZ* s = m_2c->m_8;
    if (s != 0 && s->vtbl->IsLost(s) == 0) {
        return 1;
    }
    IDirectDrawSurfaceZ* r = m_2c->m_8;
    // Named local before `== 0` so MSVC emits the setcc form (xor/test/sete/mov),
    // not the neg/sbb/inc normalize. docs/patterns/return-bool-via-local-setcc.md.
    i32 hr = r->vtbl->Restore(r);
    return hr == 0;
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

// ---------------------------------------------------------------------------
// 0x164180: draw a small crosshair marker centred at (x,y) when (x,y) is at least
// 4 px inside the surface. Locks the held surface, writes the 3-px horizontal arm
// (value 0, the centre pixel left untouched) then the 3-px-up + 3-px-down vertical
// arms (value 0xff), then unlocks. The pitch is re-read through this->m_2c inside
// the vertical loops (the base writes may alias the surface header), which is why
// each iteration reloads m_2c->pitch. __thiscall, 2 stack args (ret 0x8).
// @early-stop
// 75.05% - logic/CFG/offsets/calls/the m_2c->pitch reloads all reproduced. Residual
// is a regalloc coin-flip: retail pins x in ebx (delayed `push ebp` as a loop scratch
// after the Lock), we pin x in ebp, which cascades the register operand through the
// body; plus retail coalesces the two [off+1]/[off+2] zero stores into one word store
// while we keep three byte stores. Not source-steerable; deferred to the final sweep.
RVA(0x00164180, 0xcd)
void CDDrawSurfacePair::DrawCross(i32 x, i32 y) {
    if (x - 4 < 0) {
        return;
    }
    if (x + 4 >= m_10) {
        return;
    }
    if (y - 4 < 0) {
        return;
    }
    if (y + 4 >= m_14) {
        return;
    }
    char* base = (char*)m_2c->Lock(0);
    if (base == 0) {
        return;
    }
    i32 off = ((DrawSurfaceView*)m_2c)->m_bpp * x + ((DrawSurfaceView*)m_2c)->m_pitch * y;

    // horizontal arm (0), centre pixel skipped
    i32 i;
    char* p = base + off - 1;
    for (i = 0; i < 3; ++i) {
        *p = 0;
        --p;
    }
    base[off + 1] = 0;
    base[off + 2] = 0;
    base[off + 3] = 0;

    // vertical arm up (0xff)
    i32 up = off;
    for (i = 0; i < 3; ++i) {
        up -= ((DrawSurfaceView*)m_2c)->m_pitch;
        base[up] = (char)0xff;
    }
    // vertical arm down (0xff)
    i32 down = off;
    for (i = 0; i < 3; ++i) {
        down += ((DrawSurfaceView*)m_2c)->m_pitch;
        base[down] = (char)0xff;
    }

    DrawSurfaceView* sv = (DrawSurfaceView*)m_2c;
    sv->m_8->vtbl->Unlock(sv->m_8, 0);
}

// ---------------------------------------------------------------------------
// 0x164660: surface-lost probe (the RestoreIfLost twin).  With no surface, report
// "needs work" (1).  Otherwise, if the held IDirectDrawSurface is present and not
// lost (IsLost @+0x60 == DD_OK), report 1; else attempt Restore (@+0x6c) twice,
// reporting 1 on the first failure, and 0 only when both restores succeed.
// __thiscall, no args.  The chained `||` gives retail's shared return-1 tail.
// @early-stop
// 99.09% — every code byte matches; residual is the register the m_2c->m_8
// re-read lands in (retail reuses esi for the last block, our cl picks edx) — the
// same non-steerable regalloc coin-flip as the sibling RestoreIfLost (0x163f00,
// 98.67%).  docs/patterns/reread-member-view-pointer.md / zero-register-pinning.md.
RVA(0x00164660, 0x46)
i32 CDDrawSurfacePair::Probe_164660() {
    return m_2c == 0 ||
           (m_2c->m_8 != 0 && m_2c->m_8->vtbl->IsLost(m_2c->m_8) == 0) ||
           m_2c->m_8->vtbl->Restore(m_2c->m_8) == 0 ||
           m_2c->m_8->vtbl->Restore(m_2c->m_8) == 0;
}
