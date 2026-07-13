// DDPageMgr.cpp - the primary-surface / display-mode bring-up + Smacker playback obj.
//
// waveM-mech merged the 0x17c040-0x17d720 .text (ONE original TU: text A-B-A weave -
// CDDPageMgr's Init@0x17c040 + CheckMode16/RemoveAt/FreeAll@0x17d2b0-0x17d6b0 bracket
// the CMoviePlayer(smackervideowindow) + CDDScreen(ddscreen) methods; DDPageMgr's Init
// calls CDDScreen::HandleError@0x17cc80 - same obj). Absorbed the ex ddpagemgr +
// smackervideowindow + ddscreen units.
//
// DEFERRED FOLD (@identity-TODO): CDDPageMgr (<DDrawMgr/DirectDrawMgr.h>) and CDDScreen
// (<DDrawMgr/DDScreen.h>) are two cross-header views of the SAME tiled-DirectDraw
// display object (Init@CDDPageMgr calls HandleError@CDDScreen); the declared-only
// cross-calls stay reloc-masked (distinct mangled names), so co-locating is byte-
// preserving, but unifying the two header views is deferred work.
//
// BOUNDARY (left separate, frag-woven strays out of scope): CMoviePlayer::Open@0x17c6f0
// (movieplayer), CDDScreen::UploadPalette@0x17ca10 (palettecopy), CDDScreen::ResetPalette
// @0x17ca60 (palettereset), CImageProbe::Init@0x17cbe0 (imageprobe), PalCache::Snapshot
// @0x17cd90 (resourceloaders) - each is a method of this obj's classes wearing a stray
// unit; SHOULD fold here in a follow-up.
//
// Include env mirrors the SmackerVideoWindow environment (Mfc.h/afxwin.h -> CWnd,
// smack.h -> the RAD Smacker SDK, ddraw.h) plus the DDrawMgr display headers. Only
// offsets + code bytes are load-bearing; field names are placeholders.
#include <Mfc.h> // CString + windows.h (afx-first)
#ifdef __clang__
// The label-step clang can't parse MFC's afxwin1.inl (implicit-int CMenu::operator==);
// skip the *.inl for clang only - docs/patterns/afxwin-clang-label-step-skip-inl.md.
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h> // the REAL MFC CWnd (m_videoWnd) + AfxRegisterWndClass
#include <Ints.h>
#include <rva.h>
#include <smack.h> // the genuine RAD Smacker SDK (SMACKW32.DLL) - Smack handle + Smack* API
// smack.h pulls rad.h, whose u8/u16/u32/... object-like macros shadow <Ints.h>; undo
// them (matching-neutral: rad's u32==unsigned long is the same 4 bytes).
#undef u8
#undef u16
#undef u32
#undef u64
#undef s8
#undef s16
#undef s32
#undef s64
#include <ddraw.h> // real IDirectDraw/IDirectDrawSurface (DirectDrawCreate, IID_*, DDSURFACEDESC)
#include <Io/MoviePlayer.h>         // canonical CMoviePlayer
#include <Crypto/FecCrypt.h>        // the +0x540 decode store IS a CFecFile (Close @0x17b570)
#include <Io/FileStream.h>          // DDPageMgr's former god-TU include env
#include <DDrawMgr/DirectDrawMgr.h> // CDDPageMgr, DDModeInfo, CPageRec (canonical)
#include <DDrawMgr/DDScreen.h>      // canonical CDDScreen + CTileInfo
#include <stdio.h>
#include <string.h> // memset / inlined memcpy (rep movsd)
#include <Globals.h>

// The engine heap allocator (0x1b9b46) - Configure's explicit-blit RECT nodes.
extern "C" void* RezAlloc(u32 size);

// The dxguid GUID constants Init passes to QueryInterface by REFIID (retail .rdata
// addresses, DATA()-pinned so the `push OFFSET` reloc-masks).
DATA(0x001ef848)
extern "C" const GUID IID_IDirectDraw2; // 0x5ef848
DATA(0x001ef888)
extern "C" const GUID IID_IDirectDrawSurface3; // 0x5ef888

// The game's cached ShowCursor fn-ptr global (?::ShowCursor@@3P6GHH@ZA, def in
// stateimages) the 0x17c3f0 command handler hides the cursor through.

// [The former Handler_17c3f0 command-block view (with its ObjA2/ObjA3 placeholder-slot
// interfaces) is dissolved onto the canonical CDDScreen (<DDrawMgr/DDScreen.h>): the
// dispatches are genuine COM - IDirectDraw2::CreatePalette (slot 5, +0x14) and
// IDirectDrawSurface::SetPalette (slot 31, +0x7c) - and every touched field sits at a
// CDDScreen offset (m_dd2/m_primary/m_palette/m_palEntries/m_screenWidth/m_screenHeight/
// m_bpp). The remaining (CDDPageMgr*) casts are the documented CDDScreen==CDDPageMgr
// cross-header conflation (deferred fold).]

// ===========================================================================
// Functions in retail-RVA order.
// ===========================================================================

// CDDPageMgr::Init (0x17c040, __thiscall) - create DirectDraw, QI IDirectDraw2, set
// the cooperative level + display mode, create the primary surface (QI'd to
// IDirectDrawSurface3) and, for 8bpp, a palette; cache geometry + show the cursor.
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

// CMoviePlayer::CreateVideoWindow (0x17c2a0) - register a private window class, refuse
// if the window exists, create a screen-sized top-level MFC CWnd "Smacker Video
// Window", focus it, then hand its HWND to the host's Init.
// @early-stop
// Complete + correct (~98%). Residual is two documented walls: (1) the /GX EH scope-
// table + handler-thunk relocs are named differently ($L.../__except_list vs the retail
// Unwind@... funclet), a reloc-typing artifact; (2) MSVC5 tail-merges the two `return 0`
// guards differently than retail (one extra `jmp; xor eax,eax`).
RVA(0x0017c2a0, 0x14e)
int CMoviePlayer::CreateVideoWindow(i32 a0, i32 a1) {
    CString cls(AfxRegisterWndClass(3, 0, 0, 0));
    if (m_videoWnd != 0) {
        return 0;
    }
    m_videoWnd = new CWnd;
    if (!m_videoWnd->CreateEx(
            8,
            cls,
            "Smacker Video Window",
            0x90000000,
            0,
            0,
            GetSystemMetrics(0),
            GetSystemMetrics(1),
            0,
            0,
            0
        )) {
        return 0;
    }
    m_videoWnd->SetFocus();
    HWND h = m_videoWnd ? m_videoWnd->m_hWnd : 0;
    return Init(h, a0, a1);
}

// CDDScreen::InitMode (0x17c3f0) - the borrowed-interface mode bring-up over the
// CDDScreen display object (a stack-local 0x520+-byte block in the caller;
// CGruntzMgr::ChangeState_8fab0 builds + InitMode()s it).
RVA(0x0017c3f0, 0x14e)
i32 CDDScreen::InitMode(
    HWND wnd,
    IDirectDraw2* dd2,
    IDirectDrawSurface* primary,
    i32 p4,
    i32 p5,
    i32 height,
    i32 width,
    i32 p8,
    i32 p9,
    i32 p10,
    i32 p11,
    i32 p12,
    i32 p13,
    i32 p14,
    i32 p15,
    i32 p16,
    i32 p17,
    i32 p18,
    i32 p19,
    i32 p20,
    i32 p21,
    i32 p22,
    i32 p23,
    i32 p24,
    i32 bpp,
    i32 p26,
    i32 p27,
    i32 p28,
    i32 p29,
    i32 p30,
    i32 a31
) {
    if (!wnd || !dd2 || !primary) {
        return 0;
    }
    m_dd2 = dd2;
    m_0c = 1;
    m_primary = primary;
    Snapshot(wnd);
    if (bpp == 8) {
        if (m_dd2->CreatePalette(DDPCAPS_8BIT, m_palEntries, &m_palette, 0)) {
            HandleError();
            return 0;
        }
        m_primary->SetPalette(m_palette);
        m_510 = 0;
    }
    if (bpp == 24) {
        HandleError();
        return 0;
    }
    if (bpp == 16) {
        if (!((CDDPageMgr*)this)->CheckMode16()) {
            HandleError();
            return 0;
        }
    }
    m_screenWidth = width;
    m_screenHeight = height;
    m_bpp = bpp;
    m_window = wnd;
    m_8 = 0;
    m_srcSurf = 0;
    m_28 = 0;
    m_508 = a31;
    ::ShowCursor(0);
    m_initialized = 1;
    ((CDDPageMgr*)this)->FreeAll();
    return 1;
}

// CMoviePlayer::Teardown (0x17c510) - tear the playback object down + restore the cursor.
RVA(0x0017c510, 0x5e)
void CMoviePlayer::Teardown() {
    if (!m_active) {
        return;
    }
    CloseSmacker();
    ((CDDPageMgr*)this)->FreeAll();
    m_0 = 0; // +0x00 plain data (no vptr evidence; zeroed at teardown)
    m_active = 0;
    ((CDDScreen*)this)->HandleError();
    if (m_videoWnd) {
        m_videoWnd->DestroyWindow();
        delete m_videoWnd; // virtual dtor -> the compiler's own null-guarded slot-1 dispatch
        m_videoWnd = 0;
    }
    ShowCursor(1);
}

// CMoviePlayer::OpenLo (0x17c570) - open a Smacker stream (0xfe000 flags, +0x100000 for
// DirectSound), begin playback, roll back on failure.
RVA(0x0017c570, 0xc0)
i32 CMoviePlayer::OpenLo(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5) {
    if (!m_active) {
        return 0;
    }
    SmackSoundUseDirectSound(m_directSound);
    m_514 = a2;
    u32 flags;
    if (useDS == 1) {
        m_useDS = useDS;
        flags = 0x100000;
    } else {
        m_useDS = 0;
        flags = 0;
    }
    flags |= 0xfe000;
    m_smackHandle = SmackOpen((const char*)src, flags, -1);
    if (!m_smackHandle) {
        return 0;
    }
    m_streamOpen = 1;
    i32 r = Begin(a2, useDS, a4, a5);
    if (r) {
        return r;
    }
    if (m_24) {
        m_24->Release();
        m_24 = 0;
    }
    if (m_28) {
        m_28->Release();
        m_28 = 0;
    }
    CloseSmacker();
    return r;
}

// CMoviePlayer::OpenHi (0x17c630) - same as OpenLo but with the 0xff000 flag set.
RVA(0x0017c630, 0xc0)
i32 CMoviePlayer::OpenHi(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5) {
    if (!m_active) {
        return 0;
    }
    SmackSoundUseDirectSound(m_directSound);
    m_514 = a2;
    u32 flags;
    if (useDS == 1) {
        flags = 0x100000;
        m_useDS = useDS;
    } else {
        m_useDS = 0;
        flags = 0;
    }
    flags |= 0xff000;
    m_smackHandle = SmackOpen((const char*)src, flags, -1);
    if (!m_smackHandle) {
        return 0;
    }
    m_streamOpen = 1;
    i32 r = Begin(a2, useDS, a4, a5);
    if (r) {
        return r;
    }
    if (m_24) {
        m_24->Release();
        m_24 = 0;
    }
    if (m_28) {
        m_28->Release();
        m_28 = 0;
    }
    CloseSmacker();
    return r;
}

// CMoviePlayer::Pump (0x17c790) - pump the Win32 queue while a movie plays; abort on the
// selected key/mouse events, else render the next frame until `count` plays elapse.
RVA(0x0017c790, 0x14a)
i32 CMoviePlayer::Pump(i32 flags, i32 count) {
    if (!m_active || count < -1 || count == 0) {
        return 0;
    }
    m_loopCount = 1;
    MSG msg;
    for (;;) {
        if (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == 0x104) {
                continue;
            }
            if (msg.message == 0x105) {
                continue;
            }
            if (msg.message == 0x100) {
                if (flags & 1) {
                    return 1;
                }
                continue;
            }
            if (msg.message == 0x201 || msg.message == 0x204 || msg.message == 0x203
                || msg.message == 0x206) {
                if (flags & 0x100) {
                    return 0x100;
                }
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        } else {
            if (SmackWait(m_smackHandle)) {
                continue;
            }
            if (Frame()) {
                continue;
            }
            if (count != -1 && ++m_loopCount > count) {
                return 0x11111111;
            }
            SmackSoundOnOff(m_smackHandle, 0);
            SmackGoto(m_smackHandle, 1);
            SmackSoundOnOff(m_smackHandle, 1);
        }
    }
}

// CMoviePlayer::Advance (0x17c8e0) - wait for the stream, render a frame, loop on EOF
// until `loops` is exhausted.
RVA(0x0017c8e0, 0xca)
i32 CMoviePlayer::Advance(i32 cmd, i32 loops) {
    if (!cmd || !m_active || loops < -1 || loops == 0) {
        return 0;
    }
    i32 result = 1;
    if (m_loopCount == 0) {
        m_loopCount = result;
    }
    if (SmackWait(m_smackHandle) == 0) {
        i32 saved = m_command;
        m_command = cmd;
        result = Frame();
        if (result == 0) {
            if (loops == -1 || ++m_loopCount <= loops) {
                SmackSoundOnOff(m_smackHandle, 0);
                SmackGoto(m_smackHandle, 1);
                SmackSoundOnOff(m_smackHandle, 1);
                result = 1;
            }
        }
        m_command = saved;
    }
    if (result == 0) {
        m_loopCount = 0;
    }
    return result;
}

// CMoviePlayer::CloseSmacker (0x17c9b0) - shut the sub-player, close the stream, free buffers.
RVA(0x0017c9b0, 0x5b)
i32 CMoviePlayer::CloseSmacker() {
    if (!m_streamOpen) {
        return 0;
    }
    ((CFecFile*)&m_540)->Close();
    if (!m_smackHandle) {
        return 0;
    }
    SmackClose(m_smackHandle);
    m_smackHandle = 0;
    if (m_rezBuffer) {
        ::operator delete(m_rezBuffer);
        m_rezBuffer = 0;
    }
    m_streamOpen = 0;
    return 1;
}

// CMoviePlayer::Frame (0x17caa0) - the per-frame renderer Pump drives: lock the DDraw
// surface (retrying on DDERR_SURFACELOST), decode the current Smacker frame, blit the
// changed region(s), advance to the next frame.
RVA(0x0017caa0, 0x13b)
i32 CMoviePlayer::Frame() {
    if (m_smackHandle->NewPalette && m_520 == 8) {
        ((CDDScreen*)this)->UploadPalette();
    }
    i32 hr = m_24->Lock(0, (LPDDSURFACEDESC)m_desc, 1, 0);
    while (hr == (i32)0x887601c2) {
        if (m_24->Restore() != 0) {
            goto afterLock;
        }
        hr = m_24->Lock(0, (LPDDSURFACEDESC)m_desc, 1, 0);
    }
    if (hr == 0) {
        SmackToBuffer(m_smackHandle, 0, 0, m_lPitch, m_smackHandle->Height, m_lpSurface, m_510);
        SmackDoFrame(m_smackHandle);
        m_50c = 1;
        m_24->Unlock(m_lpSurface);
    }
afterLock:
    if (m_514 != 1) {
        while (SmackToBufferRect(m_smackHandle, 0) != 0) {
            ((CDDScreen*)this)
                ->BlitRegion(
                    m_smackHandle->LastRectx,
                    m_smackHandle->LastRecty,
                    m_smackHandle->LastRectw,
                    m_smackHandle->LastRecth
                );
        }
    } else {
        ((CDDScreen*)this)->BlitRegion(0, 0, m_smackHandle->Width, m_smackHandle->Height);
    }
    Smack* s = m_smackHandle;
    if (s->FrameNum == s->Frames - 1) {
        return 0;
    }
    SmackNextFrame(s);
    return 1;
}

// CDDScreen::HandleError (0x17cc80) - release owned interfaces; if still mid-bringup
// black the primary surface, then release the remaining objects.
RVA(0x0017cc80, 0x109)
void CDDScreen::HandleError() {
    if (m_srcSurf) {
        m_srcSurf->Release();
        m_srcSurf = 0;
    }
    if (m_28) {
        m_28->Release();
        m_28 = 0;
    }
    if (m_bpp == 8) {
        ResetPalette();
    }
    if (m_primary) {
        DDBLTFX fx;
        memset(&fx, 0, sizeof(fx));
        fx.dwSize = 0x64;
        fx.dwROP = 0x42;
        void* rc = (void*)m_primary->Blt(0, 0, 0, 0x1020000, &fx);
        if (rc) {
            memset(&fx, 0, sizeof(fx));
            fx.dwSize = 0x64;
            fx.dwFillColor = 0;
            m_primary->Blt(0, 0, 0, 0x1000400, &fx);
        }
    }
    if (m_0c == 0) {
        if (m_palette) {
            m_palette->Release();
            m_palette = 0;
        }
        if (m_primary) {
            m_primary->Release();
            m_primary = 0;
        }
        if (m_20) {
            m_20->Release();
            m_20 = 0;
        }
        if (m_dd2) {
            m_dd2->RestoreDisplayMode();
            m_dd2->Release();
            m_dd2 = 0;
        }
        if (m_dd) {
            m_dd->Release();
            m_dd = 0;
        }
    }
}

// CDDScreen::BlitRegion (0x17cdf0) - blit the (col,row,nCols,nRows) tile region from the
// source surface onto the primary; handle DDERR_SURFACELOST by restoring + retrying.
RVA(0x0017cdf0, 0x1c6)
i32 CDDScreen::BlitRegion(i32 col, i32 row, i32 nCols, i32 nRows) {
    RECT dst, src;
    if (m_destRect) {
        dst.left = m_destRect->left;
        dst.top = m_destRect->top;
        dst.right = m_destRect->right;
        dst.bottom = m_destRect->bottom;
    } else {
        dst.left = col * m_tilesAcross + m_originX;
        dst.top = row * m_tilesDown + m_originY;
        dst.right = nCols * m_tilesAcross + dst.left;
        dst.bottom = nRows * m_tilesDown + dst.top;
    }
    src.left = col;
    src.top = row;
    src.right = col + nCols;
    src.bottom = row + nRows;

    for (;;) {
        i32 hr;
        if (m_tilesAcross == 1 && m_tilesDown == 1 && m_destRect == 0) {
            hr = m_primary->BltFast(dst.left, dst.top, m_srcSurf, &src, 0x10);
            if (hr != 0x887601c2) {
                return hr;
            }
            if (m_primary->IsLost() == 0x887601c2 && m_primary->Restore() == 0) {
                if (m_bpp == 8) {
                    m_primary->SetPalette(m_palette);
                    UploadPalette();
                }
            } else {
                hr = m_srcSurf->IsLost();
                if (hr != 0x887601c2) {
                    return hr;
                }
                hr = m_srcSurf->Restore();
                if (hr != 0) {
                    return hr;
                }
            }
        } else {
            hr = m_primary->Blt(&dst, m_srcSurf, &src, 0x1000000, 0);
            if (hr != 0x887601c2) {
                return hr;
            }
            if (m_primary->IsLost() == 0x887601c2 && m_primary->Restore() == 0) {
                if (m_bpp == 8) {
                    m_primary->SetPalette(m_palette);
                    UploadPalette();
                }
            } else {
                hr = m_srcSurf->IsLost();
                if (hr != 0x887601c2) {
                    return hr;
                }
                hr = m_srcSurf->Restore();
                if (hr != 0) {
                    return hr;
                }
            }
        }
    }
}

// CDDScreen::Configure (0x17cfc0) - derive the tile grid + scroll origin for a layout
// `mode` (0..3), validating the caller's optional origin/clip against the screen first.
// @early-stop
// reloc-mask scoring artifact (~97.4%): the CODE BYTES are byte-exact (llvm-objdump -dr
// base vs target). The residual is two differently-named reloc operands: the rel32 call
// to the sibling grid validator at 0x17cbe0 (stubbed ?Unmatched_17cbe0, modeled here as
// CDDScreen::CheckGrid), and the switch jump table ($L385 vs switchdataD_0057d2a0).
// topic:scoring-artifact - no further code change possible.
RVA(0x0017cfc0, 0x2dd)
i32 CDDScreen::Configure(i32 mode, i32 flags, POINT* origin, RECT* rect) {
    if (origin) {
        if (origin->x > m_screenWidth) {
            return 0;
        }
        if (origin->y > m_screenHeight) {
            return 0;
        }
    }
    if (rect) {
        if (rect->left > rect->right) {
            return 0;
        }
        if (rect->top > rect->bottom) {
            return 0;
        }
        if ((u32)rect->right > m_screenWidth) {
            return 0;
        }
        if ((u32)rect->bottom > m_screenHeight) {
            return 0;
        }
    }
    if (m_tileInfo->m_width > m_screenWidth) {
        return 0;
    }
    if (m_tileInfo->m_height > m_screenHeight) {
        return 0;
    }
    if (!CheckGrid()) {
        return 0;
    }

    switch (mode) {
        case 0:
            m_tilesAcross = m_screenWidth / m_tileInfo->m_width;
            m_tilesDown = m_screenHeight / m_tileInfo->m_height;
            if (flags & 0x10) {
                if (!origin) {
                    return 0;
                }
                m_originX = origin->x;
                m_originY = origin->y;
            } else {
                m_originX = (m_screenWidth - m_tilesAcross * m_tileInfo->m_width) >> 1;
                m_originY = (m_screenHeight - m_tilesDown * m_tileInfo->m_height) >> 1;
            }
            break;
        case 1:
            m_tilesAcross = 1;
            m_tilesDown = 1;
            if (flags & 0x10) {
                if (!origin) {
                    return 0;
                }
                m_originX = origin->x;
                m_originY = origin->y;
            } else {
                m_originX = (m_screenWidth - m_tileInfo->m_width) >> 1;
                m_originY = (m_screenHeight - m_tileInfo->m_height) >> 1;
            }
            break;
        case 2:
            if (m_screenWidth % m_tileInfo->m_width == 0
                && m_screenHeight % m_tileInfo->m_height == 0) {
                m_tilesAcross = m_screenWidth / m_tileInfo->m_width;
                m_tilesDown = m_screenHeight / m_tileInfo->m_height;
                if (flags & 0x10) {
                    if (!origin) {
                        return 0;
                    }
                    m_originX = origin->x;
                    m_originY = origin->y;
                } else {
                    m_originX = (m_screenWidth - m_tilesAcross * m_tileInfo->m_width) >> 1;
                    m_originY = (m_screenHeight - m_tilesDown * m_tileInfo->m_height) >> 1;
                }
            } else {
                m_tilesAcross = 1;
                m_tilesDown = 1;
                m_originX = 0;
                m_originY = 0;
                m_destRect = (RECT*)RezAlloc(0x10);
                m_destRect->top = 0;
                m_destRect->left = 0;
                m_destRect->bottom = m_screenHeight;
                m_destRect->right = m_screenWidth;
                m_514 = 1;
            }
            break;
        case 3: {
            m_tilesAcross = 1;
            m_tilesDown = 1;
            m_originX = 0;
            m_originY = 0;
            if (!rect) {
                return 0;
            }
            RECT* r = (RECT*)RezAlloc(0x10);
            m_destRect = r;
            r->left = rect->left;
            r->top = rect->top;
            r->right = rect->right;
            r->bottom = rect->bottom;
            break;
        }
        default:
            return 0;
    }

    if (m_forceSingleRow != 0) {
        m_tilesDown = 1;
    }
    m_50c = 0;
    m_86a0 = 0;
    return 1;
}

// CDDPageMgr::CheckMode16 (0x17d2b0) - popcount the current display mode's R/G/B masks
// and classify a 16-bit mode (5/5/5 -> 0x80000000, 5/6/5 -> 0xc0000000).
// @early-stop
// 83.9% - logic/CFG/COM call/popcount loops/classification reproduced. The residual is a
// regalloc coin-flip: retail spills `this` (sub esp,0x70) + uses ebx as a bit counter,
// while we keep `this` in ebx (sub esp,0x6c) + use edi for the third counter. Deferred.
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

// CDDPageMgr::RemoveAt (0x17d600) - drop the 1-based idx-th CPageRec: free its three
// owned buffers, shift the tail down one slot, decrement the count, free the record.
// @early-stop
// constant-materialization wall (~86%): logic + layout byte-correct, but retail hoists
// the null `0` into edi (callee-saved) and reuses it for all 7 pointer checks/stores,
// forcing idx into esi; MSVC5 here emits `test`/immediate-0 and keeps idx in edi.
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
        ::operator delete(rec->m_00);
        rec->m_00 = 0;
    }
    if (rec->m_10) {
        ::operator delete(rec->m_10);
        rec->m_10 = 0;
    }
    if (rec->m_14) {
        ::operator delete(rec->m_14);
        rec->m_14 = 0;
    }
    i32 n = m_count - idx;
    CPageRec** dst = &m_data[idx - 1];
    if (n) {
        memcpy(dst, dst + 1, n * sizeof(CPageRec*));
    }
    m_count--;
    ::operator delete(rec);
    return 1;
}

// CDDPageMgr::FreeAll (0x17d6b0) - RemoveAt(1) every record, then free the array buffer.
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
        ::operator delete(m_data);
        m_data = 0;
    }
    m_8698 = 0;
    m_count = 0;
    return 1;
}

// CMoviePlayer::PlayList (0x17d720) - play the whole m_868c clip playlist `loops` times.
RVA(0x0017d720, 0x188)
i32 CMoviePlayer::PlayList(i32 loops) {
    if (!m_active || loops < -1 || loops == 0) {
        return 0;
    }
    i32 iter = 1;
    do {
        for (i32 i = 0; i < m_868c.GetSize(); i++) {
            PLAYLISTINFOSTRUCT* clip = m_868c[i]; // inline CArray::operator[] == m_pData[i]
            if (clip->m_src == 0) {
                return 0;
            }
            if (clip->m_openArg == 0) {
                if (OpenLo(clip->m_src, clip->m_08, clip->m_useDS, clip->m_10, clip->m_14) == 0) {
                    return 0;
                }
            } else {
                if (Open(
                        clip->m_src,
                        clip->m_openArg,
                        clip->m_08,
                        clip->m_useDS,
                        clip->m_10,
                        clip->m_14
                    )
                    == 0) {
                    return 0;
                }
            }
            PLAYLISTINFOSTRUCT* c2 = m_868c[i];
            i32 result = Pump(c2->m_flags, c2->m_count);
            if (result != 0x11111111) {
                CloseSmacker();
                return result;
            }
            if (m_command != 0) {
                DDBLTFX fx;
                memset(&fx, 0, sizeof(fx));
                fx.dwSize = sizeof(fx);
                fx.dwROP = 0x42;
                i32 hr = ((IDirectDrawSurface*)m_command)->Blt(0, 0, 0, 0x1020000, &fx);
                if (hr != 0) {
                    memset(&fx, 0, sizeof(fx));
                    fx.dwSize = sizeof(fx);
                    fx.dwFillColor = 0;
                    ((IDirectDrawSurface*)m_command)->Blt(0, 0, 0, 0x1000400, &fx);
                }
            }
            CloseSmacker();
        }
        iter++;
    } while (iter <= loops);
    return 0x11111111;
}
