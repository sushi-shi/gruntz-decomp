#include <rva.h>
#include <Rez/RezAlloc.h> // RezAlloc/RezFree
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawFrameNode.h> // the CDDrawWorkerObj element-array view
#include <DDrawMgr/DDSurface.h> // the held CDDSurface (m_surface) full def (Lock/BltFast/IsValid/m_8/m_pitch/m_b0)
#include <Gruntz/ParseSource.h> // CParseSource (LoadImage's byte-reader arg: GetEntryTag/BeginParse/EndParse)
#include <Win32.h>              // windows.h base types (ddraw.h needs them first)
#include <ddraw.h>              // real IDirectDrawSurface dispatch (IsLost/Restore/Unlock/GetCaps)
#include <string.h>             // memset for the edge-row fills (inline rep-stos CRT)
#include <stdio.h>              // sprintf (DrawCount's itoa)
#include <DDrawMgr/DirectDrawMgr.h>       // canonical CDirectDrawMgr (CreatePoolItem/CreateDevice)
#include <DDrawMgr/DDrawWorkerMapSmall.h> // CDDrawWorkerMapSmall (hoisted; meat here)
#include <DDrawMgr/DDrawWorkerList.h>     // CDDrawWorkerList (hoisted; teardown here)
#include <DDrawMgr/DDrawWorkerNode.h>     // CDDrawWorkerBase/A/B (Plot/helpers here)
#include <DDrawMgr/DDrawWorkerCtx.h>      // shared CDDrawWorkerCtx (the +0x0c owner context)
#include <DDrawMgr/DDrawWorkerCache.h>    // CDDrawWorkerCache (CreateWorker here)
#include <DDrawMgr/DDrawWorker.h>   // CDDrawWorker (the registry map values, DestroyAll's delete)
#include <DDrawMgr/AnimWorkerObj.h> // AnimWorkerObj (the 0x17c worker CreateWorker news)
#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSurfaceChildA (SetGeom_1646b0 here)
#include <Io/FileMem.h>                // CFileMem/CFileMemBase (the runtime core here)
#include <Gruntz/AniElement.h>         // CAniElement + CAniRecordView (the ANI section here)
#include <Globals.h>                   // g_aniParsedNameLen (the record-parse cursor)
#include <Gruntz/MapStringToOb.h>
#include <Gruntz/String.h>
#include <Mfc.h>
#include <Gruntz/ResolveNode.h>           // canonical CResolveNode (Init here, 0x1647e0)
#include <Image/ImageSet.h>               // CImageSet (FindKeyOfValue's target)
#include <DDrawMgr/DDrawWorkerRegistry.h> // canonical CDDrawWorkerRegistry (2 teardown fns here)
#include <DDrawMgr/DDrawSurfaceMgr.h>     // canonical CDDrawSurfaceMgr (m_mgr / m_0c parent)
#include <DDrawMgr/DDrawPtrCollections.h> // canonical CDDrawPtrCollections (the +0x1c surface pool)

// The locked-surface pixel geometry is read straight off the held CDDSurface:
// its byte-pitch (m_pitch @+0x20), its bytes-per-pixel divisor (m_b0 @+0xb0), and
// its held IDirectDrawSurface (m_8 @+0x08, for Unlock). No facet view needed.
//
// The parent manager m_mgr (pair +0x0c) / m_0c (child +0x0c) points at IS the
// canonical CDDrawSurfaceMgr; its surface pool IS m_ptrColl (+0x1c,
// CDDrawPtrCollections). The former per-TU views (CDDrawSurfaceMgrT / CDDrawSurfacePool
// onto them (2026-07-14): pool +0x1c = m_ptrColl, caps +0x34 = m_flags, hWnd/device
// +0x30 = m_hWnd, mgr-err +0x38 = m_lastError, pool-err +0x944 = m_944, and the fake
// pixel-format chain +0x04 -> +0x10 -> +0x2c is m_drawTarget -> m_frontPair -> m_surface.
// CreatePoolItem/CreateDevice remain (CDirectDrawMgr*) casts on m_ptrColl (the
// documented CDDrawPtrCollections==CDirectDrawMgr manager-unification @identity-TODO).

RVA(0x0003a1d0, 0x1d)
void CDDrawSurfacePair::BltSelf(CDDrawSurfacePair* src) {
    m_surface->BltFast(0, 0, src->m_surface, &src->m_srcRect, 0x10);
}

RVA(0x0006b270, 0x1b)
::CObject* CAniElement::AtChecked(i32 i) const {
    if (i >= 0 && i < m_records.GetSize()) {
        return m_records.GetAt(i);
    }
    return 0;
}

RVA(0x00163bc0, 0x2c)
void CDDrawWorkerList::DestroyWorkers() {
    POSITION pos = m_workers.GetHeadPosition();
    while (pos) {
        CDDrawWorkerBase* child = static_cast<CDDrawWorkerBase*>(m_workers.GetNext(pos));
        if (child) {
            delete child;
        }
    }
    m_workers.RemoveAll();
}

RVA(0x00163bf0, 0x6d)
void CDDrawWorkerList::PruneWorkers(CDDrawSurfacePair* a, CDDrawSurfacePair* b) {
    POSITION pos = m_workers.GetHeadPosition();
    while (pos) {
        POSITION cur = pos;
        CDDrawWorkerBase* child = static_cast<CDDrawWorkerBase*>(m_workers.GetNext(pos));
        child->RenderFrame(a, b);
        child->m_refCount--;
        if ((b->m_surface != 0 && (b->m_flags & 0x20000) == 0) || child->m_refCount <= 0) {
            m_workers.RemoveAt(cur);
            if (child) {
                delete child;
            }
        }
    }
}

RVA(0x00163c60, 0x2c)
void CDDrawWorkerList::ClearWorkers() {
    POSITION pos = m_workers.GetHeadPosition();
    while (pos) {
        CDDrawWorkerBase* child = static_cast<CDDrawWorkerBase*>(m_workers.GetNext(pos));
        if (child) {
            delete child;
        }
    }
    m_workers.RemoveAll();
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
        m_surface =
            static_cast<CDDSurface*>(mgr->m_ptrColl
                ->CreatePoolItem(static_cast<void*>(mgr->m_drawTarget->m_frontPair->m_surface), reinterpret_cast<void*>(4)));
        if (m_surface == 0) {
            if (m_mgr->m_lastError == 0) {
                m_mgr->m_lastError = 0xfa3;
            }
            return 0;
        }
    }
    if (m_status != 1) {
        if (m_flags & 0x10000) {
            m_surface = m_mgr->m_ptrColl->MakeAndAddB(w, h, 0, 0, -1);
        } else {
            m_surface = m_mgr->m_ptrColl->CreateB(w, h, 0, 0, -1);
        }
        if (m_surface == 0) {
            if (m_mgr->m_lastError == 0) {
                m_mgr->m_lastError = 0xfa4;
            }
            return 0;
        }
    }
    m_ownsSurface = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// 0x163db0 (slot 11): adopt an existing (already-created) surface - cache its
// geometry (w/h from the surface desc, bpp from its raw bit depth) + a {0,0,w,h}
// src rect, mark active (m_status=0x63), latch the surface (not owned). Rejects a
// null surface or non-positive geometry. __thiscall, 1 arg (ret 4).
// @early-stop
// ~77.7% - logic/CFG/offsets/store-order all byte-faithful. Residual is the mirror-
// register wall: retail keeps the `src` pointer in edx (loaded for the null test,
// reused as the geometry base, stored to m_surface LAST); MSVC on this identical
// source pins src in eax and schedules the m_surface store early. Same values/order.
// Not source-steerable (permuter 180-iter marginal). docs/patterns/zero-register-pinning.md.
RVA(0x00163db0, 0x64)
i32 CDDrawSurfacePair::InitFromSurface(CDDSurface* src) {
    if (src != 0) {
        i32 w = src->m_width;
        i32 bpp = src->m_bitDepth;
        i32 h = src->m_height;
        if (w > 0 && h > 0) {
            m_width = w;
            m_srcRect[2] = w;
            m_bpp = bpp;
            m_height = h;
            m_srcRect[0] = 0;
            m_srcRect[1] = 0;
            m_srcRect[3] = h;
            m_status = 0x63;
            m_surface = src;
            m_ownsSurface = 0;
            return 1;
        }
    }
    return 0;
}

RVA(0x00163e20, 0x2d)
void CDDrawSurfacePair::TeardownSurface() {
    if (m_surface != 0 && m_ownsSurface != 0) {
        CDDrawPtrCollections* pool = m_mgr->m_ptrColl;
        pool->RemoveItemA(m_surface);
        m_surface = 0;
    }
    m_width = 0;
}

RVA(0x00163e50, 0x8b)
i32 CDDrawSurfacePair::LoadImage(CParseSource* src) {
    i32 type;
    switch (static_cast<u32>(src->GetEntryTag())) {
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
    i32 r = m_surface->Resolve(static_cast<void*>(m_mgr->m_ptrColl), reinterpret_cast<void*>(buf), type, src->m_length, 0);
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
    IDirectDrawSurface* s = m_surface->m_ddSurface;
    if (s != 0 && s->IsLost() == 0) {
        return 1;
    }
    IDirectDrawSurface* r = m_surface->m_ddSurface;
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
    char* base = reinterpret_cast<char*>(m_surface->Lock(0));
    if (base == 0) {
        return;
    }
    CDDSurface* sv = m_surface;
    u8 c = static_cast<u8>(color);
    i32 w = right - left + 1;

    // ---- top + bottom horizontal edges ----
    if (m_bpp == 0x10) {
        i32 n = 2 * w;
        if (n > 0) {
            memset(base + sv->m_bytesPerPixel * left + sv->m_pitch * top, c, n);
        }
        if (n > 0) {
            memset(base + sv->m_bytesPerPixel * left + sv->m_pitch * bottom, c, n);
        }
    } else {
        if (w > 0) {
            memset(base + sv->m_bytesPerPixel * left + sv->m_pitch * top, c, w);
        }
        if (w > 0) {
            memset(base + sv->m_bytesPerPixel * left + sv->m_pitch * bottom, c, w);
        }
    }

    // ---- left + right vertical edges ----
    i32 h = bottom - top + 1;
    if (h > 0) {
        for (i32 y = 0; y < h; ++y) {
            if (m_bpp == 0x10) {
                i32 lo = (top + y) * sv->m_pitch + sv->m_bytesPerPixel * left;
                base[lo] = c;
                base[lo + 1] = c;
                i32 ro = (top + y) * sv->m_pitch + sv->m_bytesPerPixel * right;
                base[ro] = c;
                base[ro + 1] = c;
            } else {
                i32 lo = (top + y) * sv->m_pitch + sv->m_bytesPerPixel * left;
                base[lo] = c;
                i32 ro = (top + y) * sv->m_pitch + sv->m_bytesPerPixel * right;
                base[ro] = c;
            }
        }
    }

    // Unlock the held surface.
    sv->m_ddSurface->Unlock(0);
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
    char* base = reinterpret_cast<char*>(m_surface->Lock(0));
    if (base == 0) {
        return;
    }
    i32 off = m_surface->m_bytesPerPixel * x + m_surface->m_pitch * y;

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
        base[up] = static_cast<char>(0xff);
    }
    // vertical arm down (0xff)
    i32 down = off;
    for (i = 0; i < 3; ++i) {
        down += m_surface->m_pitch;
        base[down] = static_cast<char>(0xff);
    }

    CDDSurface* sv = m_surface;
    sv->m_ddSurface->Unlock(0);
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
i32 CDDrawSurfacePair::SetGeom(i32 w, i32 h, i32 bpp) {
    if (m_width != w || m_height != h || m_bpp != bpp) {
        i32 sysmem;
        if (m_status == 2) {
            DDSCAPS caps;
            if (0 == m_surface->m_ddSurface->GetCaps(&caps)) {
                sysmem = 0x800 & caps.dwCaps;
            } else {
                sysmem = 0;
            }
        }
        m_mgr->m_ptrColl->RemoveItemA(m_surface);
        m_surface = 0;
        if (m_status == 1) {
            CDDrawSurfaceMgr* mgr = m_mgr;
            m_surface =
                static_cast<CDDSurface*>(mgr->m_ptrColl
                    ->CreatePoolItem(static_cast<void*>(mgr->m_drawTarget->m_frontPair->m_surface), reinterpret_cast<void*>(4)));
            if (m_surface == 0) {
                return 0;
            }
        }
        if (m_status != 1) {
            if (sysmem != 0) {
                m_surface = m_mgr->m_ptrColl->MakeAndAddB(w, h, bpp, 0, -1);
            } else {
                m_surface = m_mgr->m_ptrColl->CreateB(w, h, bpp, 0, -1);
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
// The counter-draw pair (0x164380 / 0x164420), re-homed from
// src/Gruntz/ResourceLoaders.cpp: retail birth-positions both dead-center in the
// CDDrawSurfacePair block (between SetGeom and the 0x1644a0 mode-surface
// creator), so they compile in this TU (wave1-C).
// [SETTLED (was @identity-TODO): the ResLoaders::DrawHost views WERE this class -
// their +0x2c "counter window" is m_surface (CDDSurface*), whose +0x08 (m_8) is the
// DC-capable IDirectDrawSurface; GetDC (slot 17, +0x44) / ReleaseDC (slot 26, +0x68)
// __thiscall(rc, n): print n centred into rc using the held surface's DC.
RVA(0x00164380, 0x98)
void CDDrawSurfacePair::DrawCount(RECT* rc, i32 n) {
    char buf[0x20];
    sprintf(buf, "%i", n);
    CDDSurface* w = m_surface;
    if (!w) {
        return;
    }
    HDC hdc = 0;
    w->m_ddSurface->GetDC(&hdc);
    if (!hdc) {
        return;
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, 0xffffff);
    DrawTextA(hdc, buf, strlen(buf), rc, 0x25);
    w->m_ddSurface->ReleaseDC(hdc);
}

RVA(0x00164420, 0x79)
void CDDrawSurfacePair::DrawLabel(RECT* rc, char* text) {
    CDDSurface* w = m_surface;
    if (!w) {
        return;
    }
    HDC hdc = 0;
    w->m_ddSurface->GetDC(&hdc);
    if (!hdc) {
        return;
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, 0xffffff);
    DrawTextA(hdc, text, strlen(text), rc, 0x25);
    w->m_ddSurface->ReleaseDC(hdc);
}

// ---------------------------------------------------------------------------
// 0x1644a0: CDDrawSurfaceChildA::SetGeometry (vtable slot-9 override; the raw
// 0x1eff70[9] holds this rva - the old CDDrawSurfacePair binding contradicted the
// pair's own vtable, whose [9] is the inherited 0x158fd0). Create the DirectDraw
// mode surface: cache {w,h,bpp}, build the device
// surface through the pool (mode 0x11 for w>320 else 0x51; fullscreen bit from
// mgr->m_flags), then attach + validate it. Each failure path stashes an error code
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
i32 CDDrawSurfaceChildA::SetGeometry(i32 w, i32 h, i32 bpp) {
    CDDrawSurfaceMgr* mgr = OwnerMgr();
    m_width = w;
    m_height = h;
    m_bpp = bpp;
    CDDrawPtrCollections* pool = mgr->m_ptrColl;
    i32 mode = 0x11;
    if (w <= 0x140) {
        mode = 0x51;
    }
    i32 hr;
    if (mgr->m_flags & 0x10) {
        hr = pool->CreateDevice(static_cast<void*>(mgr->m_hWnd), reinterpret_cast<void*>(2), w, h, bpp, mode);
    } else {
        hr = pool->CreateDevice(static_cast<void*>(mgr->m_hWnd), static_cast<void*>(0), w, h, bpp, mode);
    }
    if (hr == 0) {
        i32 err = pool->m_lastError;
        if (err != 0) {
            switch (err) {
                case 0x3e9: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80e9;
                    }
                    return 0;
                }
                case 0x3ea: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80ea;
                    }
                    return 0;
                }
                case 0x3eb: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80eb;
                    }
                    return 0;
                }
                case 0x3ec: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80ec;
                    }
                    return 0;
                }
                case 0x3ed: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80ed;
                    }
                    return 0;
                }
            }
            CDDrawSurfaceMgr* md = OwnerMgr();
            if (md->m_lastError == 0) {
                md->m_lastError = 0xbb9;
            }
            return 0;
        }
        CDDrawSurfaceMgr* m4 = OwnerMgr();
        if (m4->m_lastError == 0) {
            m4->m_lastError = 0xbb9;
        }
        return 0;
    }
    CDDrawSurfaceMgr* m2 = OwnerMgr();
    i32 amode = 1;
    if (m2->m_flags & 2) {
        amode = 2;
    }
    CDDSurface* surf = pool->Createab8_24_3(amode);
    m_surface = surf;
    if (surf != 0 && surf->IsValid()) {
        return 1;
    }
    CDDrawSurfaceMgr* m3 = OwnerMgr();
    if (m3->m_lastError == 0) {
        m3->m_lastError = 0xbba;
    }
    return 0;
}

RVA(0x00164650, 0x3)
void CDDrawSurfacePair::BlitDirtyRect(CDDrawSurfacePair* other, i32* pos, i32* size) {}

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
i32 CDDrawSurfacePair::Probe() {
    return m_surface == 0 || (m_surface->m_ddSurface != 0 && m_surface->m_ddSurface->IsLost() == 0)
           || m_surface->m_ddSurface->Restore() == 0 || m_surface->m_ddSurface->Restore() == 0;
}

void operator delete(void*);

RVA(0x001646b0, 0xde)
i32 CDDrawSurfaceChildA::SetGeom(i32 w, i32 h, i32 bpp) {
    if (m_width == w && m_height == h && m_bpp == bpp) {
        return 1;
    }
    CDDrawPtrCollections* pool = OwnerMgr()->m_ptrColl;
    if (pool == 0) {
        return 0;
    }
    pool->RemoveItemA(m_surface);
    m_surface = 0;
    if (pool->ConfigureSurface(w, h, bpp, 0, 0) != 0) {
        return 0;
    }
    i32 amode = 1;
    if (OwnerMgr()->m_flags & 2) {
        amode = 2;
    }
    m_surface = pool->Createab8_24_3(amode);
    if (m_surface == 0) {
        return 0;
    }
    if (!m_surface->IsValid()) {
        return 0;
    }
    if (w > 0 && h > 0 && (bpp == 8 || bpp == 16 || bpp == 24 || bpp == 32)) {
        m_bpp = bpp;
        m_width = w;
        m_height = h;
        m_srcRect[0] = 0;
        m_srcRect[1] = 0;
        m_srcRect[2] = w;
        m_srcRect[3] = h;
        return 1;
    }
    return 0;
}

// CResolveNode::SetPosition (0x164790, vtable slot 9 of ??_7CResolveNode - the
// datum holds this RVA at +0x24): set position + reset the draw state; seeds m_3c
// off the +0x0c owner-ctx handle. Called directly (rel32) by the wide-object
// Setup @0x150d60 AND by every worker Vfunc* (qualified base call) - the proof
// that unmasked the ex "CDDrawWorkerBase::Helper_164790" label.
// @early-stop
// regalloc/scheduling wall (topic:regalloc): logic + every member store are
// byte-exact, but retail parks the ctx handle in eax while cl parks it in
// edx and reuses one `mov eax,1` for both m_50 and the return. ~90%.
RVA(0x00164790, 0x41)
i32 CResolveNode::SetPosition(i32 x, i32 y) {
    m_screenX = x;
    m_10 = 0;
    m_14 = 0;
    m_stateFlags = 0;
    m_44 = 0;
    m_drawFillArg = 0;
    m_drawActive = 0;
    m_screenY = y;
    m_48 = 0x32;
    m_drawFillCmd = 1;
    m_level = OwnerMgr()->m_level; // the mgr's +0x24 CGameLevel
    return 1;
}

RVA(0x001647e0, 0x48)
i32 CResolveNode::Init(
    i32 owner,
    i32 field04,
    i32 resolveX,
    i32 resolveY,
    i32 field40,
    i32 field08
) {
    m_0c = owner;
    m_04 = field04;
    m_flags = field08;
    m_drawFillArg = 0;
    m_drawActive = 0;
    m_drawFillCmd = 1;
    SetPosition(resolveX, resolveY); // virtual slot 9 (0x164790)
    m_stateFlags = field40;
    return 1;
}

RVA(0x00165210, 0xa2)
void CDDrawWorkerRegistry::DestroyAll() {
    ::CObject* val = 0;
    POSITION pos = reinterpret_cast<POSITION>((m_10map.GetCount() != 0 ? -1 : 0));
    CString key;
    if (*reinterpret_cast<volatile i32*>(&pos) != 0) {
        do {
            m_10map.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete (static_cast<CDDrawWorker*>(val)); // the map values ARE the keyed workers
            }
        } while (pos != 0);
    }
    m_10map.RemoveAll();
}

static inline i32 ReadWorkerCacheField1c(const CDDrawWorkerCache* p) {
    return *reinterpret_cast<const i32*>((reinterpret_cast<const char*>(p) + 0x1c));
}
static inline AnimWorkerObj* MakeAnimWorker(const CDDrawWorkerCache* parent) {
    AnimWorkerObj* w = new AnimWorkerObj;
    if (w != 0) {
        i32 field1c = ReadWorkerCacheField1c(parent);
        i32 surfaceMgr = parent->m_0c;
        w->m_04 = field1c;
        w->m_08 = 0;
        w->m_0c = reinterpret_cast<CDDrawSurfaceMgr*>(surfaceMgr);
        w->m_notify = 0;
        w->m_payload = 0;
        w->m_logic = 0;
        w->m_target = 0;
        w->m_1c = 0;
        w->m_targetId = 0;
        w->m_payloadSize = 0;
    }
    return w;
}
RVA(0x001652c0, 0x92)
void* CDDrawWorkerCache::CreateWorker(GameObjNotifyFn factory, const char* key, i32 a3) {
    AnimWorkerObj* w = MakeAnimWorker(this);

    if (w->Init(factory, a3) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    m_10[key] = static_cast<::CObject*>(w);
    return w;
}

void* operator new(u32 n);

RVA(0x00165360, 0xf1)
CString CDDrawWorkerCache::FindKeyOfValue(::CObject* target) {
    ::CObject* val = 0;
    POSITION pos = m_10.GetStartPosition();
    CString key;
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        // Reads the raw +0x10 dword of both, exactly as retail does (this is the ex
        // `m_poolItems` view of that offset). NOTE, flagged not fixed: +0x10 is the embedded
        // ::CObArray's VPTR, so for two CObArray-holding objects this compare is a
        // constant-vs-itself and the scan returns the first key. That is pre-existing
        // and out of this fold's scope - the fold only preserves the same memory read.
        if (val != 0 && *reinterpret_cast<i32*>(&(static_cast<CImageSet*>(val))->m_items) == *reinterpret_cast<i32*>(&(static_cast<CImageSet*>(target))->m_items)) {
            return key;
        }
    }
    CString empty;
    return empty;
}

// ---------------------------------------------------------------------------
// 0x165460: (re)build the element from a parsed-source descriptor. __thiscall,
// 3 stack args (ret 0xc). Returns 1 on success, 0 on any record-parse failure.
// @early-stop
// 89.34% - whole body byte-correct in shape (offsets, calls, control flow, the
// for-loop success-first/`jl` exit order per docs/patterns/loop-preheader-vs-exit-
// block-order.md). Residual is three regalloc/scheduling walls: (1) retail rebases
// the descriptor's m_08 read onto the cursor (`mov ecx,[ebp-0x18]` where ebp=src+0x20)
// vs our `mov ecx,[ebx+8]`; (2) retail re-zeros the record reg at the loop top
// (`xor edi,edi` + a `jmp` skipping it the first iteration - zero-register-pinning.md);
// (3) the SetAtGrow arg lands in edx (retail) vs ecx (ours) with the array-`this`
// `lea` hoisted before the pushes (pin-local-for-callee-saved-reg.md). None steerable.
RVA(0x00165460, 0x156)
i32 CAniElement::Build(void* ctx, CAniSource* src, i32 flags) {
    m_flags = flags;
    m_scale = 1.0f;
    m_total = 0;
    m_flags = src->m_flags | flags;

    const char* cursor = src->m_data;
    if (src->m_namelen != 0) {
        m_name = static_cast<char*>(operator new(src->m_namelen + 2));
        i32 n;
        for (n = 0; n < src->m_namelen; n++) {
            m_name[n] = *cursor++;
        }
        m_name[n] = 0;
    } else {
        m_name = 0;
    }

    CAniRecordView* rec = 0;
    i32 i;
    for (i = 0; i < src->m_count; i++) {
        rec = new CAniRecordView;
        if (rec->Parse(ctx, reinterpret_cast<const i16*>(cursor)) == 0) {
            goto fail;
        }
        m_records.SetAtGrow(m_records.GetSize(), static_cast<::CObject*>(rec));
        cursor += g_aniParsedNameLen + 0x14;
        m_total += rec->GetSize();
    }
    return 1;

fail:
    if (rec != 0) {
        delete rec;
    }
    for (i = 0; i < m_records.GetSize(); i++) {
        ::CObject* p = m_records.GetAt(i);
        if (p != 0) {
            delete (static_cast<CAniRecordView*>(p));
        }
    }
    if (m_name != 0) {
        RezFree(m_name);
        m_name = 0;
    }
    m_records.SetSize(0, -1);
    return 0;
}

RVA(0x001655c0, 0x53)
i32 CAniElement::Configure(void* ctx, void* entry, i32 flags) {
    if ((static_cast<CParseSource*>(entry))->GetEntryTag() != 0x414e49) {
        return 0;
    }
    m_flags = flags;
    void* src = reinterpret_cast<void*>((static_cast<CParseSource*>(entry))->BeginParse());
    if (src == 0) {
        return 0;
    }
    i32 r = Build(ctx, static_cast<CAniSource*>(src), 0);
    (static_cast<CParseSource*>(entry))->EndParse();
    return r;
}

SIZE_UNKNOWN(CAniRecordArray);
SIZE_UNKNOWN(CAniRecordView);
SIZE_UNKNOWN(CAniSource);

// ---------------------------------------------------------------------------
// 0x165620: load + build the element from a file.  Open the reader on `filename`;
// on failure return 0.  Otherwise read the whole file into a RezAlloc'd buffer,
// hand it to Build(ctx, buf, 0), free the buffer, and return the build
// result (the reader local is destroyed on every exit).  __thiscall, ret 0xc.
// @early-stop
// 97.29% — the whole body is byte-faithful (Open/GetLength/RezAlloc/Read/Build/
// RezFree + the three reader-dtor cleanup tails + the /GX frame).  Residual is the
// EH scope-table cookie (retail push 0x8 / Unwind@005e2410 vs our push 0x0 / $L
// funclet) + the reloc-masked names (retail's reader is CFileIO with a virtual
// dtor; modeling that is matching-neutral, tested).  docs/patterns/gx-scoped-local-
// eh-frame-size.md.
RVA(0x00165620, 0x101)
i32 CAniElement::LoadFile(void* ctx, void* filename, i32 a3) {
    CFile fr;
    if (fr.Open(static_cast<const char*>(filename), 0, 0) == 0) {
        return 0;
    }
    u32 size = fr.GetLength();
    void* buf = RezAlloc(size);
    if (fr.Read(buf, size) == 0) {
        RezFree(buf);
        return 0;
    }
    i32 r = Build(ctx, static_cast<CAniSource*>(buf), 0);
    RezFree(buf);
    return r;
}

RVA(0x00165730, 0x4c)
void CAniElement::DeleteAll() {
    for (i32 i = 0; i < m_records.GetSize(); i++) {
        CAniRecordView* el = static_cast<CAniRecordView*>(m_records.GetAt(i));
        if (el != 0) {
            delete el;
        }
    }
    if (m_name != 0) {
        RezFree(m_name);
        m_name = 0;
    }
    m_records.SetSize(0, -1); // CObArray::RemoveAll (inlined as SetSize(0,-1))
}

RVA(0x00165810, 0xa9)
void CDDrawWorkerMapSmall::DestroyAll() {
    CObject* val = 0;
    POSITION pos = reinterpret_cast<POSITION>((m_map1.GetCount() != 0 ? -1 : 0));
    CString key;
    if (*reinterpret_cast<volatile i32*>(&pos) != 0) {
        do {
            m_map1.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete (static_cast<CAniRecordBase2*>(val));
            }
        } while (pos != 0);
    }
    m_map1.RemoveAll();
    m_cachedWorker = 0; // +0x64 (the teardown nulls it - a pointer, not a counter)
}

// 0x1658c0: lock the surface arg; on 0 bail. Build a worker, dispatch its +0x28
// virtual with (data, a3); unlock. On failure destroy the worker and return 0; on
// success store it in m_map1 under `key` (or the surface name) and return it.
// @early-stop
// vptr-scheduler wall (~96%): the real CAniRecordBase2(field04, field0c) ctor
// (docs/patterns/ctor-vptr-interleave-vs-spelled-out-init.md) fixed the construction
// regalloc; the residual is the vptr store position (cl 1st vs retail 4th) + the /GX
// EH-state schedule around the destructible worker/CString locals.
RVA(0x001658c0, 0xcc)
void* CDDrawWorkerMapSmall::Factory_1658c0(CParseSource* a1, const char* key, i32 a3) {
    i32 data = a1->BeginParse();
    if (data == 0) {
        return 0;
    }
    CAniRecordBase2* w = new CAniRecordBase2(m_map1.GetCount(), m_0c);
    if (w->AllocBufMakeB(data, a3) == 0) {
        a1->EndParse();
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    a1->EndParse();
    const char* k = key != 0 ? key : a1->m_name;
    char buf[0x40];
    strcpy(buf, k);
    m_map1[buf] = static_cast<CObject*>(w);
    return w;
}

// Allocate + construct a worker, call its +0x28 virtual with (arg1, arg3).
// @early-stop
// vptr-scheduler wall (see docs/patterns/ctor-vptr-interleave-vs-spelled-out-init.md):
// the real ctor fixed the regalloc; residual is only the vptr store position (cl 1st vs
// retail 4th).
RVA(0x00165990, 0x77)
void* CDDrawWorkerMapSmall::CreateWorker28(i32 a1, const char* key, i32 a3) {
    CAniRecordBase2* w = new CAniRecordBase2(m_map1.GetCount(), m_0c);
    if (w->AllocBufMakeB(a1, a3) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    m_map1[key] = static_cast<CObject*>(w);
    return w;
}

// As CreateWorker28 but dispatches the worker's +0x2c virtual.
// @early-stop
// vptr-scheduler wall (see docs/patterns/ctor-vptr-interleave-vs-spelled-out-init.md):
// residual is only the vptr store position (cl 1st vs retail 4th).
RVA(0x00165a10, 0x77)
void* CDDrawWorkerMapSmall::CreateWorker2C(i32 a1, const char* key, i32 a3) {
    CAniRecordBase2* w = new CAniRecordBase2(m_map1.GetCount(), m_0c);
    if (w->AllocBufMakeB2(a1, a3) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    m_map1[key] = static_cast<CObject*>(w);
    return w;
}

// 0x165a90: require the surface's format id to be 0x504358; lock it, bail on 0.
// Build a worker, dispatch its +0x30 virtual with (data, a1, a3).
// @early-stop
// vptr-scheduler wall (~93%, twin of Factory_1658c0): real ctor fixed the regalloc;
// residual is the vptr store position (cl 1st vs retail 4th) + the /GX EH-state schedule.
RVA(0x00165a90, 0xf4)
void* CDDrawWorkerMapSmall::Factory_165a90(CParseSource* a1, i32 a2, i32 a3) {
    if (a1->GetEntryTag() != 0x504358) {
        return 0;
    }
    i32 data = a1->BeginParse();
    if (data == 0) {
        return 0;
    }
    const char* keyHandle = reinterpret_cast<const char*>(a1->m_length); // +0x0c doubles as the key handle for this entry kind
    CAniRecordBase2* w = new CAniRecordBase2(m_map1.GetCount(), m_0c);
    if (w->AllocBufMakeB3(data, reinterpret_cast<i32>(a1), a3) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    const char* k = keyHandle != 0 ? keyHandle : a1->m_name;
    char buf[0x40];
    strcpy(buf, k);
    m_map1[buf] = static_cast<CObject*>(w);
    return w;
}

// 0x165b90: map teardown twin of DestroyAll (0x165810). /GX EH frame.
// @early-stop
// EH-frame register-schedule wall (~82%): logic byte-faithful; the residual is the
// TU-context EH-state/val-slot schedule + the reloc-masked EH-state push.
RVA(0x00165b90, 0xa9)
void CDDrawWorkerMapSmall::ResetSlots() {
    CObject* val = 0;
    POSITION pos = reinterpret_cast<POSITION>((m_map1.GetCount() != 0 ? -1 : 0));
    CString key;
    if (*reinterpret_cast<volatile i32*>(&pos) != 0) {
        do {
            m_map1.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete (static_cast<CAniRecordBase2*>(val));
            }
        } while (pos != 0);
    }
    m_map1.RemoveAll();
    m_cachedWorker = 0; // +0x64 (the teardown nulls it - a pointer, not a counter)
}

RVA(0x00165c40, 0xe7)
i32 CDDrawWorkerMapSmall::RemoveByValue(CObject* obj) {
    if (m_cachedWorker == obj) {
        m_cachedWorker = 0;
    }
    CObject* val = 0;
    POSITION pos = reinterpret_cast<POSITION>((m_map1.GetCount() != 0 ? -1 : 0));
    CString key;
    while (*reinterpret_cast<volatile i32*>(&pos) != 0) {
        m_map1.GetNextAssoc(pos, key, val);
        if (val == obj) {
            m_map1.RemoveKey(key);
            if (obj != 0) {
                delete (static_cast<CAniRecordBase2*>(obj));
            }
            return 1;
        }
    }
    return 0;
}

// 0x165d30 (__thiscall): remove a worker from m_map1 by its `key`.
// @early-stop
// ~77.7% - MSVC5 `delete`-null-check wall: retail runs the value's scalar-deleting
// destructor DIRECTLY with no null-check; `delete w` (the only MSVC5-expressible
// form) emits an unconditional null-check + reloads val.
RVA(0x00165d30, 0x5f)
i32 CDDrawWorkerMapSmall::RemoveByKey(const char* key) {
    CObject* val = 0;
    m_map1.Lookup(key, val);
    if (val == 0) {
        return 0;
    }
    CAniRecordBase2* w = static_cast<CAniRecordBase2*>(val);
    if (m_cachedWorker == val) {
        m_cachedWorker = 0;
    }
    m_map1.RemoveKey(key);
    delete w;
    return 1;
}

RVA(0x00165e30, 0x27)
i32 CFileMemBase::SetName(const char* name, i32 a, i32 b) {
    m_name = name;
    m_mode = a;
    m_4 = b;
    return 1;
}

RVA(0x00165e60, 0x82)
i32 CFileMem::Open() {
    if (m_name.GetLength() == 0) {
        return 0;
    }

    // Through a CFile* so MSVC5 keeps the retail virtual dispatch (an object
    // receiver would devirtualize; the ex-CFileIODispatch view faked these slots).
    CFile* io = &m_file;
    if (WantRead()) {
        if (!io->Open(m_name, 0, 0)) {
            return 0;
        }
        m_length = io->GetLength();
        m_offset = 0;
        return 1;
    }

    if (!io->Open(m_name, 0x1001, 0)) {
        return 0;
    }
    m_length = 0;
    m_offset = 0;
    return 1;
}

RVA(0x00165ef0, 0xf)
i32 CFileMem::Ready() {
    CFile* io = &m_file;
    io->Close();
    return 1;
}

RVA(0x00165f00, 0x48)
i32 CFileMem::Read(void* buf, i32 n) {
    if (buf == 0) {
        return 0; // (no-op path; eax left = buf = 0 in retail)
    }
    if (n == 0) {
        return 0;
    }
    CFile* io = &m_file;
    if (io->Read(buf, n) != static_cast<u32>(n)) {
        return 0;
    }
    m_offset += n;
    return 1;
}

RVA(0x00165f50, 0x45)
i32 CFileMem::Write(const void* buf, i32 n) {
    if (buf == 0) {
        return 0; // (no-op path; eax left untouched in retail)
    }
    if (n == 0) {
        return 0;
    }
    CFile* io = &m_file;
    io->Write(buf, n);
    m_length += n;
    m_offset += n;
    return 1;
}

// ===========================================================================
// The CDDrawWorkerA/B plot/draw T-pair (0x165fa0-0x1660b0) + Helper.
// ===========================================================================
// 0x165fa0 (vtable slot 10): plot the worker's marker pixel (m_78) at pixel
// (m_5c, m_60) onto BOTH passed surface pairs - the back one (b) first.
// @early-stop
// ~89% spill-slot regalloc wall: the second block is byte-exact; the first
// differs only in WHICH coordinate is spilled across the Lock call.
RVA(0x00165fa0, 0x93)
void CDDrawWorkerA::RenderFrame(CDDrawSurfacePair* a, CDDrawSurfacePair* b) {
    {
        i32 x = m_screenX;
        char c = m_78b;
        CDDSurface* s = b->m_surface;
        i32 y = m_screenY;
        char* base = reinterpret_cast<char*>(s->Lock(0));
        if (base != 0) {
            base[s->m_bytesPerPixel * x + s->m_pitch * y] = c;
            s->m_ddSurface->Unlock(0);
        }
    }
    {
        char c = m_78b;
        i32 y = m_screenY;
        i32 x = m_screenX;
        CDDSurface* s = a->m_surface;
        char* base = reinterpret_cast<char*>(s->Lock(0));
        if (base != 0) {
            base[s->m_bytesPerPixel * x + y * s->m_pitch] = c;
            s->m_ddSurface->Unlock(0);
        }
    }
}

// look up a named object in the owner's map, then fetch element[idx] when in
// range; cache it at m_78 and return whether it is non-null.
// @early-stop
// scheduling wall (topic:regalloc): body byte-exact; only residue is WHERE the
// Lookup out-param zero-init lands (a 1-instruction reorder). ~95%.
RVA(0x00166040, 0x66)
i32 CDDrawWorkerB::Helper(i32 key, i32 idx) {
    CObject* obj = 0;
    OwnerMgr()->m_imageRegistry->m_10map.Lookup(reinterpret_cast<const char*>(key), obj);
    CDDrawWorkerObj* p = reinterpret_cast<CDDrawWorkerObj*>(obj);
    i32 v;
    if (p != 0 && idx >= p->m_64 && idx <= p->m_68) {
        v = reinterpret_cast<i32>(p->m_14[idx]);
    } else {
        v = 0;
    }
    m_78 = v;
    return v != 0;
}

// The render object CDDrawWorkerB::m_78 holds IS a CImage frame (the collection
// elements are CImages, vftable 0x5eaa2c - see CDDrawWorker::InsertFrame): its
// slot 14 (+0x38) is CImage::RenderImage @0x153470, the blit-mode/clip selector.
// Kept as this TU's dispatch view pending the fold (@identity-TODO): CImage.h
// (The ex-CDDrawFrameNode dispatch view is DISSOLVED: m_frame IS a CImage - its
// vtable 0x1eaa2c is the ground truth - and slot 14 is the real
// CImage::RenderImage(CResolveNode*, CDDrawSurfacePair*); the worker IS-A
// CResolveNode, so the dispatch is now spelled through the real class.)

RVA(0x001660b0, 0x33)
void CDDrawWorkerB::RenderFrame(CDDrawSurfacePair* a, CDDrawSurfacePair* b) {
    m_frame->RenderImage(this, a);
    if (b->m_surface != 0 && (b->m_flags & 0x20000) == 0) {
        m_frame->RenderImage(this, b);
    }
}