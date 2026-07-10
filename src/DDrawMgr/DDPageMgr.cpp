// DDPageMgr.cpp - CDDPageMgr (DDrawMgr), the primary-surface / display-mode bring-up
// class behind the second DirectDrawCreate caller (0x17c040). One contiguous retail
// .text block (0x17c040-0x17d720) == one retail .obj. De-fragmented: Init (0x17c040)
// and CheckMode16 (0x17d2b0) rejoined here from the DirectDrawMgr god-TU so all four
// CDDPageMgr methods (Init, CheckMode16, RemoveAt, FreeAll) live in their own .obj.
//
// The class is modeled canonically in <DDrawMgr/DirectDrawMgr.h> (formerly a divergent
// .cpp-local view here); it owns its OWN IDirectDraw + IDirectDraw2 + primary surface +
// palette (Init) plus, far down, a 1-based growable array of CPageRec records at +0x8690
// (RemoveAt/FreeAll). On a failed COM call it routes through its own HandleError, not
// CDirectDrawMgr::GetErrorString. Only offsets + code bytes are load-bearing.
//
// Include environment mirrors the former god-TU (Mfc.h via FileStream.h -> windows.h
// before <ddraw.h>) so Init/CheckMode16 keep matching.
#include <Io/FileStream.h>

#include <DDrawMgr/DirectDrawMgr.h> // CDDPageMgr, DDModeInfo, CPageRec (canonical)
#include <ddraw.h> // real DirectDraw SDK (DirectDrawCreate, IDirectDraw/2, IID_*, DDSURFACEDESC)
#include <rva.h>
#include <stdio.h>
#include <string.h> // memset / inlined memcpy (rep movsd)
#include <Globals.h>

// The Rez heap free (0x1b9b82, __cdecl); reloc-masked (RemoveAt/FreeAll).
extern "C" void RezFree(void* p);

// The dxguid GUID constants Init passes to QueryInterface by REFIID. <ddraw.h> declares
// them (EXTERN_C const GUID); redeclared with DATA() to bind their retail .rdata address
// so the `push OFFSET` reloc-masks (IID_IDirectDrawSurface3 is Init's local reference).
DATA(0x001ef848)
extern "C" const GUID IID_IDirectDraw2; // 0x5ef848
DATA(0x001ef888)
extern "C" const GUID IID_IDirectDrawSurface3; // 0x5ef888

// ===========================================================================
// CDDPageMgr (DDrawMgr) - the primary-surface bring-up (second DirectDrawCreate
// caller). On a failed COM call it routes through its own HandleError, not
// GetErrorString.
// ===========================================================================

// CDDPageMgr::Init (__thiscall, ret 0xc => 3 args). Creates its own DirectDraw,
// QueryInterfaces IDirectDraw2, sets the cooperative level + display mode,
// creates the primary surface (QI'd to IDirectDrawSurface3) and, for 8bpp, a
// palette; caches the geometry and shows the cursor. The desc scratch is filled
// through the named DDSURFACEDESC fields (m_descSize / ddsCaps.dwCaps m_descCaps).
RVA(0x0017c040, 0x25d)
i32 CDDPageMgr::Init(void* window, DDModeInfo* mode, u32 coopFlags) {
    if (m_initialized != 0) {
        return 0;
    }
    if (window == 0) {
        return 0;
    }

    i32 w, h, bpp;
    if (mode != 0) {
        w = mode->width;
        h = mode->height;
        bpp = mode->bpp;
    } else {
        w = 0x280;
        h = 0x1e0;
        bpp = 8;
    }

    m_c = 0;
    if (DirectDrawCreate(0, &m_dd1, 0) != 0) {
        return 0;
    }
    if (m_dd1->QueryInterface(IID_IDirectDraw2, (void**)&m_dd2) != 0) {
        return 0;
    }
    if (m_dd2->SetCooperativeLevel((HWND)window, coopFlags) != 0) {
        HandleError();
        return 0;
    }
    if (m_dd2->SetDisplayMode(w, h, bpp, 0, 0) != 0) {
        HandleError();
        return 0;
    }

    i32 i;
    i32* d = (i32*)m_desc;
    for (i = 0x1b; i != 0; i--) {
        *d++ = 0;
    }
    m_descSize = 0x6c;
    m_24 = 1;
    m_descCaps = 0x200; // ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE
    if (m_dd2->CreateSurface((LPDDSURFACEDESC)m_desc, &m_primarySurfaceRaw, 0) != 0) {
        HandleError();
        return 0;
    }

    if (m_primarySurfaceRaw->QueryInterface(IID_IDirectDrawSurface3, (void**)&m_primarySurface)
        != 0) {
        return 0;
    }

    OnModeSet(w);

    if (mode->bpp == 8) {
        if (m_dd2->CreatePalette(4, (LPPALETTEENTRY)m_palEntries, &m_palette, 0) != 0) {
            HandleError();
            return 0;
        }
        m_primarySurface->SetPalette(m_palette);
        m_modeTag = 0;
    }

    if (mode->bpp == 0x18) {
        HandleError();
        return 0;
    }
    if (mode->bpp == 0x10) {
        if (CheckMode16() == 0) {
            HandleError();
            return 0;
        }
    }

    m_width = w;
    m_24 = 0;
    m_28 = 0;
    m_height = h;
    m_bpp = bpp;
    m_window = window;
    m_c = 0;
    ShowCursor(0);
    m_initialized = 1;
    FreeAll();
    return 1;
}

// ---------------------------------------------------------------------------
// CDDPageMgr::CheckMode16 (@0x17d2b0, __thiscall, no args)
// Read the current display mode's pixel format (IDirectDraw2::GetDisplayMode,
// vtbl slot 12), popcount its R/G/B channel bit-masks, and classify a 16-bit
// mode: 5/5/5 -> tag m_modeTag = 0x80000000, 5/6/5 -> 0xc0000000. Returns 1 on a
// recognised 16-bit mode, 0 otherwise (incl. a failed GetDisplayMode).
// @early-stop
// 83.9% - logic/CFG/the GetDisplayMode COM call/the three 32-iter popcount loops/
// the 5-5-5 vs 5-6-5 classification are all reproduced. The residual is a regalloc
// coin-flip: retail spills `this` to a stack slot (sub esp,0x70) and uses ebx as a
// bit counter, while we keep `this` in ebx (sub esp,0x6c) and use edi for the third
// counter; this cascades the loop register operands + the desc stack offset.
// Not source-steerable; deferred to the final sweep.
RVA(0x0017d2b0, 0xfa)
i32 CDDPageMgr::CheckMode16() {
    DDSURFACEDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = 0x6c;
    if (m_dd2->GetDisplayMode(&desc) != 0) {
        return 0;
    }

    i32 r = 0;
    i32 g = 0;
    i32 b = 0;
    i32 i;
    u32 m;

    m = desc.ddpfPixelFormat.dwRBitMask;
    for (i = 0; i < 32; i++) {
        if ((m & 1) == 1) {
            r++;
        }
        m >>= 1;
    }
    m = desc.ddpfPixelFormat.dwGBitMask;
    for (i = 0; i < 32; i++) {
        if ((m & 1) == 1) {
            g++;
        }
        m >>= 1;
    }
    m = desc.ddpfPixelFormat.dwBBitMask;
    for (i = 0; i < 32; i++) {
        if ((m & 1) == 1) {
            b++;
        }
        m >>= 1;
    }

    if (r == 5 && g == 5 && b == 5) {
        m_modeTag = (i32)0x80000000;
        return 1;
    }
    if (r == 5 && g == 6 && b == 5) {
        m_modeTag = (i32)0xc0000000;
        return 1;
    }
    return 0;
}

// ===========================================================================
// 0x17d600 - RemoveAt(idx): drop the 1-based idx-th record. Free its three owned
// buffers, shift the tail down one slot, decrement the count, free the record.
// ===========================================================================
// @early-stop
// constant-materialization wall: logic + layout byte-correct, but retail hoists
// the null `0` into edi (callee-saved) and reuses it for all 7 pointer
// null-checks/stores (`cmp [..],edi` / `mov [..],edi`), which forces idx into esi;
// MSVC5 here emits `test`/immediate-0 and keeps idx in edi instead. The whole
// register allocation cascades from that one materialization choice; not
// source-steerable (an explicit `void* z=0` folds back). ~86%.
RVA(0x0017d600, 0xad)
i32 CDDPageMgr::RemoveAt(i32 idx) {
    if (!m_initialized) {
        return 0;
    }
    if (m_count < idx) {
        return 0;
    }
    CPageRec* rec = m_data[idx - 1];
    if (rec->m_00) {
        RezFree(rec->m_00);
        rec->m_00 = 0;
    }
    if (rec->m_10) {
        RezFree(rec->m_10);
        rec->m_10 = 0;
    }
    if (rec->m_14) {
        RezFree(rec->m_14);
        rec->m_14 = 0;
    }
    i32 n = m_count - idx;
    CPageRec** dst = &m_data[idx - 1];
    if (n) {
        memcpy(dst, dst + 1, n * sizeof(CPageRec*));
    }
    m_count--;
    RezFree(rec);
    return 1;
}

// ===========================================================================
// 0x17d6b0 - FreeAll: RemoveAt(1) every record in turn (bailing if one fails),
// then free the array buffer and clear the bookkeeping fields.
// ===========================================================================
RVA(0x0017d6b0, 0x70)
i32 CDDPageMgr::FreeAll() {
    if (!m_initialized) {
        return 0;
    }
    i32 count = m_count;
    for (i32 i = 0; i < count; i++) {
        if (!RemoveAt(1)) {
            return 0;
        }
    }
    if (m_data) {
        RezFree(m_data);
        m_data = 0;
    }
    m_8698 = 0;
    m_count = 0;
    return 1;
}
