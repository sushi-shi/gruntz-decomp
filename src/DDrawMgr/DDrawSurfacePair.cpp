#include <rva.h>
// DDrawSurfacePair.cpp - a surface-backed drawing region in the DDrawMgr
// DDrawMgr image family. It owns one held DDraw surface (a CPoolItemA, the
// CDDSurface wrapper) borrowed from the parent CDirectDrawMgr's surface pool,
// plus a cached pixel geometry (width @+0x10 / height @+0x14 / bpp @+0x18) and an
// x/y offset window @+0x1c. Its own vtable is @0x5eff30; the grand-base dtor
// vtable is g_wapObjectDtorVtbl @0x5e8cb4. See include/DDrawMgr/DDrawSurfacePair.h.
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

#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>  // the held CDDSurface (m_surface) full def (Lock/BltFast/IsValid/m_8/m_pitch/m_b0)
#include <Gruntz/ParseSource.h>  // CParseSource (LoadImage's byte-reader arg: GetEntryTag/BeginParse/EndParse)
#include <Win32.h>  // windows.h base types (ddraw.h needs them first)
#include <ddraw.h>  // real IDirectDrawSurface dispatch (IsLost/Restore/Unlock/GetCaps)
#include <string.h> // memset for the edge-row fills (inline rep-stos CRT)
class CDDSurface;
class CDDrawPtrCollections {
public:
    void RemoveItemA(CDDSurface* i);
    CDDSurface* MakeAndAddB(i32 a, i32 b, i32 c, i32 d, i32 e);
    CDDSurface* CreateB(i32 a, i32 b, i32 c, i32 d, i32 e);
    CDDSurface* Createab8_24_3(i32 m);
};
#include <DDrawMgr/DirectDrawMgr.h> // canonical CDirectDrawMgr (CreatePoolItem/CreateDevice)

// The locked-surface pixel geometry is read straight off the held CDDSurface:
// its byte-pitch (m_pitch @+0x20), its bytes-per-pixel divisor (m_b0 @+0xb0), and
// its held IDirectDrawSurface (m_8 @+0x08, for Unlock). No facet view needed.

// ---------------------------------------------------------------------------
// The parent manager m_mgr points at, and its surface pool: only the offsets
// Create()/1644a0 read are pinned. These are owner-TU-only views (CDDrawSurfaceMgr
// is forward-declared, pointer-only, in the shared DDrawSurfacePair.h). The pool
// callees are reloc-masked __thiscall engine callees modeled as methods on a tiny
// view so `mov ecx,pool; call` falls out with no caller-side stack cleanup.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CDDrawSurfacePool);
class CDDrawSurfacePool {
public:
    // RemoveItemA @0x142160 IS CDDrawPtrCollections::RemoveItemA; cast at the call.
    // AcquireA @0x143630 IS CDirectDrawMgr::CreatePoolItem; cast at the call.
    // MakeAndAddB @0x142e60 IS CDDrawPtrCollections::MakeAndAddB; cast at the call.
    // CreateB @0x1423c0 IS CDDrawPtrCollections::CreateB; cast at the call.
    // CreateModeSurface @0x141dc0 IS CDirectDrawMgr::CreateDevice; cast at the call.
    // AttachMode @0x142b70 IS CDDrawPtrCollections::Createab8_24_3; cast at the call.
    char _pad0[0x944];
    i32 m_lastError; // +0x944  last DirectDraw error stash
};

// The pixel-format chain m_fmtChain walks: +0x04 -> +0x10 -> +0x2c.
SIZE_UNKNOWN(CDDrawSurfChainB);
struct CDDrawSurfChainB {
    char _pad0[0x2c];
    i32 m_pixelFormat; // +0x2c  pixel-format token
};
SIZE_UNKNOWN(CDDrawSurfChainA);
struct CDDrawSurfChainA {
    char _pad0[0x10];
    CDDrawSurfChainB* m_next; // +0x10
};

// CDDrawSurfaceMgr - the parent manager view m_mgr points at (owner-TU view; the
// shared header forward-declares it). A pixel-format chain at +0x4, the surface pool
// at +0x1c, device/caps at +0x30/+0x34, and a last-error word at +0x38.
SIZE_UNKNOWN(CDDrawSurfaceMgr);
struct CDDrawSurfaceMgr {
    char _pad0[0x04];
    CDDrawSurfChainA* m_fmtChain; // +0x04  pixel-format chain
    char _pad8[0x1c - 0x08];
    CDDrawSurfacePool* m_pool; // +0x1c  surface pool
    char _pad20[0x30 - 0x20];
    i32 m_device;    // +0x30  device/context handle
    i32 m_capsFlags; // +0x34  caps flags (bit4 = fullscreen, bit1 = double-buffer)
    i32 m_lastError; // +0x38  last-error word
};

// ---------------------------------------------------------------------------
// 0x03a1d0: BltFast `src`'s held surface onto ours at (0,0) with the source's
// offset window (&src->m_srcRect) as the src rect and DDBLTFAST_SRCCOLORKEY|WAIT
// (0x10) flags.  __thiscall, one ptr arg.
RVA(0x0003a1d0, 0x1d)
void CDDrawSurfacePair::BltSelf(CDDrawSurfacePair* src) {
    m_surface->BltFast(0, 0, src->m_surface, &src->m_srcRect, 0x10);
}

// ---------------------------------------------------------------------------
// 0x158fd0 (slot 9): SetGeometry - cache the {w,h,bpp} pixel geometry and a
// {0,0,w,h} src rect. Reject non-positive w/h. __thiscall, 3 args (ret 0xc).
// Shared body (ICF) with CDrawSubWorker slot 9.
// @early-stop
// 83.86% - logic/CFG/offsets/store-order all byte-exact. Residual is a regalloc
// coin-flip: retail materializes bpp up-front into the callee-saved edi (push/pop
// edi) alongside w/h; the /O2 scheduler on this identical source keeps bpp in eax
// and loads it lazily at its single m_bpp store (fewer bytes). Same values, same
// store order. Not source-steerable (permuter 200-iter no-change); the sibling
// CDrawSubWorker::SetGeom_159020 pins bpp only because its bpp-validation reads it
// early - this variant has no such read. docs/patterns/zero-register-pinning.md family.
RVA(0x00158fd0, 0x41)
i32 CDDrawSurfacePair::SetGeometry_158fd0(i32 w, i32 h, i32 bpp) {
    if (w <= 0 || h <= 0) {
        return 0;
    }
    m_height = h;
    m_srcRect[3] = h;
    m_width = w;
    m_bpp = bpp;
    m_srcRect[0] = 0;
    m_srcRect[1] = 0;
    m_srcRect[2] = w;
    return 1;
}

// ---------------------------------------------------------------------------
// 0x159090 (slot 5): IsLoaded - the surface-ready predicate: a held surface,
// a positive width, a parent manager, and an active status word. __thiscall.
RVA(0x00159090, 0x24)
i32 CDDrawSurfacePair::IsLoaded() {
    if (m_surface != 0 && m_width > 0 && m_mgr != 0 && m_status != -1) {
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x1590f0: the (non-deleting) destructor. Real polymorphic now: cl emits the
// implicit ??_7CDDrawSurfacePair own-vptr stamp in the ENTRY state (stamp-first),
// runs TeardownSurface (remove the surface from the pool), zeroes the width and
// the moved-down base fields, then the empty ~CSurfacePairBase folds in the
// implicit grand-base re-stamp last. /GX EH frame from the non-trivial base
// subobject. (eh-dtor-implicit-vptr-stamp-first.md sub-case 2.)
RVA(0x001590f0, 0x56)
CDDrawSurfacePair::~CDDrawSurfacePair() {
    TeardownSurface();
    m_width = 0;
    // base fields, moved out of ~CSurfacePairBase so they precede the grand-stamp:
    m_flags = 0;
    m_mgr = 0;
    m_status = -1;
    // empty ~CSurfacePairBase() folds the implicit grand-base re-stamp here, last.
}

// ---------------------------------------------------------------------------
// 0x163c90: Create(w, h, bpp, flags) - cache the {w,h,bpp} geometry + the
// {0,0,w,h} source rect, then acquire a held surface from the parent manager's
// pool: when m_status == 1 via AcquireA(pixel-format token, 4); otherwise via
// MakeAndAddB/CreateB (selected by the 0x10000 flag bit). On any failure record a
// last-error code (0xfa1/0xfa2/0xfa3/0xfa4) in the manager (only if still 0) and
// return 0; on success flag the surface as owned (m_ownsSurface=1) and return 1. The
// two m_status paths are TWO sequential ifs (if m_status==1 {...} if m_status!=1 {...}), which
// is why the success merge re-tests m_status. __thiscall, 4 stack args (ret 0x10).
// @early-stop
// 96.02% - logic/CFG/offsets/calls/error-codes all reproduced. The lone residual is
// a 1-instruction regalloc coin-flip in the w<=0 error path: retail loads m_status into
// eax (mov eax,[esi+4]; cmp $1,eax) so it can reuse esi for the manager, we emit the
// shorter direct compare (cmp $1,[esi+4]). Same values, same branch. Not source-
// steerable; docs/patterns/zero-register-pinning.md family.
RVA(0x00163c90, 0x116)
i32 CDDrawSurfacePair::Create(i32 w, i32 h, i32 bpp, i32 a3) {
    m_flags = a3;
    if (w <= 0 || h <= 0) {
        i32 k = m_status;
        CDDrawSurfaceMgr* mgr = m_mgr;
        if (k == 1) {
            if (mgr->m_lastError == 0) {
                mgr->m_lastError = 0xfa1;
            }
        } else {
            if (mgr->m_lastError == 0) {
                mgr->m_lastError = 0xfa2;
            }
        }
        return 0;
    }
    m_width = w;
    m_height = h;
    m_bpp = bpp;
    i32* rect = m_srcRect;
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = w;
    rect[3] = h;
    if (m_status == 1) {
        CDDrawSurfaceMgr* mgr = m_mgr;
        m_surface = (CDDSurface*)((CDirectDrawMgr*)mgr->m_pool)
                        ->CreatePoolItem((void*)mgr->m_fmtChain->m_next->m_pixelFormat, (void*)4);
        if (m_surface == 0) {
            if ((m_mgr)->m_lastError == 0) {
                (m_mgr)->m_lastError = 0xfa3;
            }
            return 0;
        }
    }
    if (m_status != 1) {
        if (m_flags & 0x10000) {
            m_surface = ((CDDrawPtrCollections*)(m_mgr)->m_pool)->MakeAndAddB(w, h, 0, 0, -1);
        } else {
            m_surface = ((CDDrawPtrCollections*)(m_mgr)->m_pool)->CreateB(w, h, 0, 0, -1);
        }
        if (m_surface == 0) {
            if ((m_mgr)->m_lastError == 0) {
                (m_mgr)->m_lastError = 0xfa4;
            }
            return 0;
        }
    }
    m_ownsSurface = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// 0x163e20: the surface teardown (vtable slot 7). When the held surface is
// present AND owned (m_ownsSurface set), remove it from the parent manager's surface pool;
// then zero m_surface. Always zero the width (m_width).
RVA(0x00163e20, 0x2d)
void CDDrawSurfacePair::TeardownSurface() {
    if (m_surface != 0 && m_ownsSurface != 0) {
        CDDrawSurfacePool* pool = m_mgr->m_pool;
        ((CDDrawPtrCollections*)pool)->RemoveItemA(m_surface);
        m_surface = 0;
    }
    m_width = 0;
}

// ---------------------------------------------------------------------------
// 0x163e50 (vtable slot 13): load an image from a parse source into the held
// surface. Read the source's 4-char magic tag ('BMP'/'PCX'/'DIR'/'DIP' -> type
// 1/2/3/4; anything else fails), pull the live source pointer (BeginParse), hand
// it to CDDSurface::Resolve(pool, buf, type, length, 0), release the parse buffer
// (EndParse), and return Resolve's status. __thiscall, 1 ptr arg (ret 0x4).
RVA(0x00163e50, 0x8b)
i32 CDDrawSurfacePair::LoadImage_163e50(CParseSource* src) {
    i32 type;
    switch ((u32)src->GetEntryTag()) {
        case 0x424d50: // 'BMP'
            type = 1;
            break;
        case 0x504358: // 'PCX'
            type = 2;
            break;
        case 0x524944: // 'DIR'
            type = 3;
            break;
        case 0x504944: // 'DIP'
            type = 4;
            break;
        default:
            return 0;
    }
    i32 buf = src->BeginParse();
    if (buf == 0) {
        return 0;
    }
    i32 r = m_surface->Resolve((void*)m_mgr->m_pool, (void*)buf, type, src->m_length, 0);
    src->EndParse();
    return r;
}

// ---------------------------------------------------------------------------
// 0x163f00: restore the held surface if it was lost. With no surface, report OK
// (1). Otherwise, when the held IDirectDrawSurface is present and not lost
// (IsLost, slot 24 @+0x60, returns DD_OK), report OK; else Restore it (slot 27
// @+0x6c) and report whether the restore succeeded. __thiscall, no args.
// @early-stop
// regalloc coin-flip (98.67%): every code byte matches retail (incl. the setcc
// boolean) EXCEPT the register the m_surface->m_8 re-read lands in (retail eax / ours
// edx) + the carried scratch reg in the setcc tail (ecx vs edx). Same values, same
// stores; not source-steerable. docs/patterns/zero-register-pinning.md.
RVA(0x00163f00, 0x40)
i32 CDDrawSurfacePair::RestoreIfLost() {
    if (m_surface == 0) {
        return 1;
    }
    IDirectDrawSurface* s = m_surface->m_8;
    if (s != 0 && s->IsLost() == 0) {
        return 1;
    }
    IDirectDrawSurface* r = m_surface->m_8;
    // Named local before `== 0` so MSVC emits the setcc form (xor/test/sete/mov),
    // not the neg/sbb/inc normalize. docs/patterns/return-bool-via-local-setcc.md.
    i32 hr = r->Restore();
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
    if (left < 0 || left >= m_width) {
        return;
    }
    i32 top = rect[1];
    if (top < 0 || top >= m_height) {
        return;
    }
    i32 right = rect[2];
    if (right < 0 || right >= m_width) {
        return;
    }
    i32 bottom = rect[3];
    if (bottom < 0 || bottom >= m_height) {
        return;
    }
    char* base = (char*)m_surface->Lock(0);
    if (base == 0) {
        return;
    }
    CDDSurface* sv = m_surface;
    u8 c = (u8)color;
    i32 w = right - left + 1;

    // ---- top + bottom horizontal edges ----
    if (m_bpp == 0x10) {
        i32 n = 2 * w;
        if (n > 0) {
            memset(base + sv->m_b0 * left + sv->m_pitch * top, c, n);
        }
        if (n > 0) {
            memset(base + sv->m_b0 * left + sv->m_pitch * bottom, c, n);
        }
    } else {
        if (w > 0) {
            memset(base + sv->m_b0 * left + sv->m_pitch * top, c, w);
        }
        if (w > 0) {
            memset(base + sv->m_b0 * left + sv->m_pitch * bottom, c, w);
        }
    }

    // ---- left + right vertical edges ----
    i32 h = bottom - top + 1;
    if (h > 0) {
        for (i32 y = 0; y < h; ++y) {
            if (m_bpp == 0x10) {
                i32 lo = (top + y) * sv->m_pitch + sv->m_b0 * left;
                base[lo] = c;
                base[lo + 1] = c;
                i32 ro = (top + y) * sv->m_pitch + sv->m_b0 * right;
                base[ro] = c;
                base[ro + 1] = c;
            } else {
                i32 lo = (top + y) * sv->m_pitch + sv->m_b0 * left;
                base[lo] = c;
                i32 ro = (top + y) * sv->m_pitch + sv->m_b0 * right;
                base[ro] = c;
            }
        }
    }

    // Unlock the held surface.
    sv->m_8->Unlock(0);
}

// ---------------------------------------------------------------------------
// 0x164180: draw a small crosshair marker centred at (x,y) when (x,y) is at least
// 4 px inside the surface. Locks the held surface, writes the 3-px horizontal arm
// (value 0, the centre pixel left untouched) then the 3-px-up + 3-px-down vertical
// arms (value 0xff), then unlocks. The pitch is re-read through this->m_surface inside
// the vertical loops (the base writes may alias the surface header), which is why
// each iteration reloads m_surface->pitch. __thiscall, 2 stack args (ret 0x8).
// @early-stop
// 75.05% - logic/CFG/offsets/calls/the m_surface->pitch reloads all reproduced. Residual
// is a regalloc coin-flip: retail pins x in ebx (delayed `push ebp` as a loop scratch
// after the Lock), we pin x in ebp, which cascades the register operand through the
// body; plus retail coalesces the two [off+1]/[off+2] zero stores into one word store
// while we keep three byte stores. Not source-steerable; deferred to the final sweep.
// (75.05->74.96 sub-0.1% wiggle when CSurfacePairBase gained its real CWapObj base:
// TU-wide symbol-set/EH-state reshuffle, not a logic change - the regalloc wall stands.)
RVA(0x00164180, 0xcd)
void CDDrawSurfacePair::DrawCross(i32 x, i32 y) {
    if (x - 4 < 0) {
        return;
    }
    if (x + 4 >= m_width) {
        return;
    }
    if (y - 4 < 0) {
        return;
    }
    if (y + 4 >= m_height) {
        return;
    }
    char* base = (char*)m_surface->Lock(0);
    if (base == 0) {
        return;
    }
    i32 off = m_surface->m_b0 * x + m_surface->m_pitch * y;

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
        up -= m_surface->m_pitch;
        base[up] = (char)0xff;
    }
    // vertical arm down (0xff)
    i32 down = off;
    for (i = 0; i < 3; ++i) {
        down += m_surface->m_pitch;
        base[down] = (char)0xff;
    }

    CDDSurface* sv = m_surface;
    sv->m_8->Unlock(0);
}

// ---------------------------------------------------------------------------
// 0x164250 (vtable slot 10): re-set the surface geometry to {w,h,bpp}. If the
// cached geometry already matches, do nothing (return 1). Otherwise, when the
// surface is in the "attached" state (m_status==2) probe whether the held surface
// lives in system memory (GetCaps & DDSCAPS_SYSTEMMEMORY); drop the current surface
// from the pool, then re-acquire one: via CreatePoolItem when m_status==1, else via
// MakeAndAddB (system-memory) / CreateB (video). Cache the new geometry + a {0,0,w,h}
// src rect and return 1 on a valid {w>0,h>0,bpp in {8,16,24,32}}. __thiscall, 3 args.
// @early-stop
// ~78.5% block-layout/tail-merge wall. Logic/CFG/offsets/calls/geometry-set all
// reproduced (incl. the bpp-hoist to ebp after the MakeAndAddB bpp-arg fix). Residual
// is MSVC5's basic-block layout: retail keeps each surface-alloc failure `return 0`
// INLINE (fall-through, reusing eax==null-surface, no xor) and places the geometry-
// equal `return 1` at a cold tail; our cl tail-MERGES the three `return 0`s into one
// shared `xor eax,eax` block and inlines the equal `return 1`. The sibling Create
// (0x163c90) hits the same family. Not source-steerable (invert-condition + permuter
// both no-op). docs/patterns/zero-register-pinning.md / tail-merge layout.
RVA(0x00164250, 0x12b)
i32 CDDrawSurfacePair::SetGeom_164250(i32 w, i32 h, i32 bpp) {
    if (m_width != w || m_height != h || m_bpp != bpp) {
        i32 sysmem;
        if (m_status == 2) {
            DDSCAPS caps;
            if (0 == m_surface->m_8->GetCaps(&caps)) {
                sysmem = 0x800 & caps.dwCaps;
            } else {
                sysmem = 0;
            }
        }
        ((CDDrawPtrCollections*)m_mgr->m_pool)->RemoveItemA(m_surface);
        m_surface = 0;
        if (m_status == 1) {
            CDDrawSurfaceMgr* mgr = m_mgr;
            m_surface = (CDDSurface*)((CDirectDrawMgr*)mgr->m_pool)
                            ->CreatePoolItem((void*)mgr->m_fmtChain->m_next->m_pixelFormat, (void*)4);
            if (m_surface == 0) {
                return 0;
            }
        }
        if (m_status != 1) {
            if (sysmem != 0) {
                m_surface = ((CDDrawPtrCollections*)m_mgr->m_pool)->MakeAndAddB(w, h, bpp, 0, -1);
            } else {
                m_surface = ((CDDrawPtrCollections*)m_mgr->m_pool)->CreateB(w, h, bpp, 0, -1);
            }
            if (m_surface == 0) {
                return 0;
            }
        }
        if (w > 0 && h > 0 && (8 == bpp || bpp == 16 || bpp == 24 || 32 == bpp)) {
            m_srcRect[0] = 0;
            m_srcRect[1] = 0;
            m_width = w;
            m_height = h;
            m_bpp = bpp;
            m_srcRect[2] = w;
            m_srcRect[3] = h;
            return 1;
        }
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x1644a0: create the DirectDraw mode surface. Cache {w,h,bpp}, build the device
// surface through the pool (mode 0x11 for w>320 else 0x51; fullscreen bit from
// mgr->m_capsFlags), then attach + validate it. Each failure path stashes an error code
// in mgr->m_lastError (only if not already set): 0x80e9..0x80ed for the five pool error
// codes, 0xbb9 for an unresolved/zero pool error, 0xbba for an attach/validate miss.
// ---------------------------------------------------------------------------
// The attached surface's readiness predicate is CDDSurface::IsValid (slot 5, @0x14).
// @early-stop
// ~87.4%: the prologue, the mode/fullscreen branches, the pool call, and all seven
// error blocks (the five 0x80e9..0x80ed switch cases + both 0xbb9 + the 0xbba) are
// byte-identical. Residual: MSVC5 cross-jumps (merges) the two byte-identical 0xbb9
// blocks (switch-default + err==0) that retail emitted as separate copies, which
// shifts the trailing success + 0xbba blocks. A block-merge/layout artifact, not a
// source lever (the two paths are genuinely identical code). Logic byte-faithful.
RVA(0x001644a0, 0x19b)
i32 CDDrawSurfacePair::directx_wrapper_caller_1644a0_DirectDrawCreate_DirectDrawEnumerateA(
    i32 w,
    i32 h,
    i32 bpp
) {
    CDDrawSurfaceMgr* mgr = m_mgr;
    m_width = w;
    m_height = h;
    m_bpp = bpp;
    CDDrawSurfacePool* pool = mgr->m_pool;
    i32 mode = 0x11;
    if (w <= 0x140) {
        mode = 0x51;
    }
    i32 hr;
    if (mgr->m_capsFlags & 0x10) {
        hr = ((CDirectDrawMgr*)pool)->CreateDevice((void*)mgr->m_device, (void*)2, w, h, bpp, mode);
    } else {
        hr = ((CDirectDrawMgr*)pool)->CreateDevice((void*)mgr->m_device, (void*)0, w, h, bpp, mode);
    }
    if (hr == 0) {
        i32 err = pool->m_lastError;
        if (err != 0) {
            switch (err) {
                case 0x3e9: {
                    CDDrawSurfaceMgr* m = m_mgr;
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80e9;
                    }
                    return 0;
                }
                case 0x3ea: {
                    CDDrawSurfaceMgr* m = m_mgr;
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80ea;
                    }
                    return 0;
                }
                case 0x3eb: {
                    CDDrawSurfaceMgr* m = m_mgr;
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80eb;
                    }
                    return 0;
                }
                case 0x3ec: {
                    CDDrawSurfaceMgr* m = m_mgr;
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80ec;
                    }
                    return 0;
                }
                case 0x3ed: {
                    CDDrawSurfaceMgr* m = m_mgr;
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80ed;
                    }
                    return 0;
                }
            }
            CDDrawSurfaceMgr* md = m_mgr;
            if (md->m_lastError == 0) {
                md->m_lastError = 0xbb9;
            }
            return 0;
        }
        CDDrawSurfaceMgr* m4 = m_mgr;
        if (m4->m_lastError == 0) {
            m4->m_lastError = 0xbb9;
        }
        return 0;
    }
    CDDrawSurfaceMgr* m2 = m_mgr;
    i32 amode = 1;
    if (m2->m_capsFlags & 2) {
        amode = 2;
    }
    CDDSurface* surf = ((CDDrawPtrCollections*)pool)->Createab8_24_3(amode);
    m_surface = surf;
    if (surf != 0 && surf->IsValid()) {
        return 1;
    }
    CDDrawSurfaceMgr* m3 = m_mgr;
    if (m3->m_lastError == 0) {
        m3->m_lastError = 0xbba;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x164650: the dirty-rect blit hook - an empty (retail `ret 0xc`) no-op. The
// CWwdGameObjectC blit dispatch (Slot34/38) calls it per (pos,size) region; the
// retail build left the body empty. __thiscall, 3 args (ret 0xc).
RVA(0x00164650, 0x3)
void CDDrawSurfacePair::BlitDirtyRect_164650(CDDrawSurfacePair* other, i32* pos, i32* size) {}

// ---------------------------------------------------------------------------
// 0x164660: surface-lost probe (the RestoreIfLost twin).  With no surface, report
// "needs work" (1).  Otherwise, if the held IDirectDrawSurface is present and not
// lost (IsLost @+0x60 == DD_OK), report 1; else attempt Restore (@+0x6c) twice,
// reporting 1 on the first failure, and 0 only when both restores succeed.
// __thiscall, no args.  The chained `||` gives retail's shared return-1 tail.
// @early-stop
// 99.09% — every code byte matches; residual is the register the m_surface->m_8
// re-read lands in (retail reuses esi for the last block, our cl picks edx) — the
// same non-steerable regalloc coin-flip as the sibling RestoreIfLost (0x163f00,
// 98.67%).  docs/patterns/reread-member-view-pointer.md / zero-register-pinning.md.
RVA(0x00164660, 0x46)
i32 CDDrawSurfacePair::Probe_164660() {
    return m_surface == 0 || (m_surface->m_8 != 0 && m_surface->m_8->IsLost() == 0)
           || m_surface->m_8->Restore() == 0 || m_surface->m_8->Restore() == 0;
}
