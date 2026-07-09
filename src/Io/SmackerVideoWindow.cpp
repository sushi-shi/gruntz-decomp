// SmackerVideoWindow.cpp - the Smacker playback host's "create the fullscreen
// video window" method (0x17c2a0), which the ApiCaller stub misfiled as
// directx_wrapper_caller_17c2a0_DirectDrawCreate. It registers a private window
// class (AfxRegisterWndClass), refuses if the window already exists, allocates +
// creates a screen-sized top-level MFC CWnd titled "Smacker Video Window", gives it
// focus, then hands its HWND to the host's Init (0x17c040).
//
// The MFC callees (AfxRegisterWndClass / CString ctor+dtor / operator new /
// CWnd ctor+CreateEx+SetFocus) are modeled as reloc-masked externs (match-by-shape);
// their real NAFXCW mangled names differ but objdiff pairs the relocs by type. The
// destructible CString local forces the /GX EH frame (this unit uses the `eh`
// profile). Field NAMES are placeholders; offsets + call-site bytes load-bearing.
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
// smack.h pulls rad.h, which defines u8/u16/u32/u64/s8/s16/s32/s64 as object-like
// macros that shadow <Ints.h>'s typedefs; undo them so the rest of this TU keeps our
// aliases (matching-neutral: rad's u32==unsigned long is the same 4 bytes).
#undef u8
#undef u16
#undef u32
#undef u64
#undef s8
#undef s16
#undef s32
#undef s64

#include <ddraw.h> // real IDirectDrawSurface (m_24/m_28: Lock/Restore/Unlock/Release)

// m_videoWnd is the REAL MFC CWnd from <afxwin.h> (the former CursSink 2-slot PMF
// view + the <Gruntz/Wnd.h> minimal view are both folded away). Layout verified
// against retail: sizeof(CWnd) == 0x3c (`new CWnd` -> push 0x3c), m_hWnd at +0x1c,
// and the two Teardown dispatches are the classic MFC teardown - slot 24 (+0x60) =
// CWnd::DestroyWindow, slot 1 (+0x04) = the scalar-deleting dtor (`delete wnd`) -
// slot indices confirmed with clang's MSVC-ABI -fdump-vtable-layouts over this
// exact toolchain header state. Ctor/CreateEx/SetFocus/DestroyWindow stay external
// NAFXCW entrypoints (reloc-masked); AfxRegisterWndClass now comes from afxwin.h.

// Smacker imports (IAT) reached during open/pump/close come from <smack.h>.
// The Rez allocator's free (RVA 0x1b9b82).
extern "C" void RezFree_call(void* p);

// m_24/m_28 are real IDirectDrawSurface COM interfaces (<ddraw.h>): the open/close
// paths Release them (slot 2 == +0x08) and the per-frame Frame() decodes into them
// via Lock (slot 25 == +0x64), Restore (slot 27 == +0x6c) and Unlock (slot 32 ==
// +0x80). The former hand-rolled SmkBuf placeholder (only Release) is dissolved into
// the real interface, so both dispatch families use ONE type, cast-free.

// The playback host IS the canonical CMoviePlayer (<Io/MoviePlayer.h>): the former
// TU-local "CSmackWin" view duplicated its +0x04 active flag / +0x540 decode store /
// the 0x17c510 Teardown + 0x17c630 OpenHi RVAs that header already claimed, and its
// "SmackSub" +0x540 sub-player was a second view of CMovieDecodeStore (Shutdown ==
// Abort @0x17b570). Both views are folded onto the canonical class (the union of
// field knowledge migrated into MoviePlayer.h).
#include <Io/MoviePlayer.h>
class CPageStore17b510 {
public:
    void Close();
};

// @early-stop
// Complete + correct (~98%). Residual is two documented walls: (1) the /GX EH
// scope-table + handler-thunk relocs are named differently ($L.../__except_list vs
// the retail Unwind@... funclet), a reloc-typing artifact; (2) MSVC5 tail-merges the
// two `return 0` guards (already-open + CreateEx-fail) differently than retail (one
// extra `jmp; xor eax,eax`). All logic + args (AfxRegisterWndClass(3), the CString,
// `new CWnd`, CreateEx(8, cls, "Smacker Video Window", 0x90000000, 0,0, GSM(0), GSM(1),
// 0,0,0), SetFocus, Init) + the GetSystemMetrics IAT-cache-in-ebp are byte-exact.
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

// __thiscall(): tear the playback object down and restore the cursor. Re-homed
// from ApiCallers (placeholder CursHost_17c510): its m_videoWnd (video window at +0x53c)
// is the same CWnd CreateVideoWindow above `new`s, so this is the same CSmackWin.
RVA(0x0017c510, 0x5e)
void CMoviePlayer::Teardown() {
    if (!m_active) {
        return;
    }
    CloseSmacker();
    Free17d6b0();
    m_0 = 0; // +0x00 plain data (no vptr evidence; zeroed at teardown)
    m_active = 0;
    Free17cc80();
    if (m_videoWnd) {
        m_videoWnd->DestroyWindow();
        delete m_videoWnd; // virtual dtor -> the compiler's own null-guarded slot-1 dispatch
        m_videoWnd = 0;
    }
    ShowCursor(1);
}

// __thiscall(src,a2,useDS,a4,a5): open a Smacker stream (0xfe000 flags, plus
// 0x100000 when DirectSound is requested), begin playback, roll back on failure.
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

// __thiscall(src,a2,useDS,a4,a5): same as OpenLo but with the 0xff000 flag set.
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

// __thiscall(flags, count): pump the Win32 queue while a Smacker movie plays; abort
// with 1/0x100 on a key/mouse event the abort flags select, else render the next
// frame and re-loop until `count` plays elapse (count==-1 loops forever).
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

// __thiscall(cmd, loops): wait for the stream, render a frame, and on EOF loop
// back to the start until `loops` is exhausted (loops==-1 loops forever).
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

// __thiscall: shut the sub-player, close the Smacker stream, free buffers.
RVA(0x0017c9b0, 0x5b)
i32 CMoviePlayer::CloseSmacker() {
    if (!m_streamOpen) {
        return 0;
    }
    ((CPageStore17b510*)&m_540)->Close();
    if (!m_smackHandle) {
        return 0;
    }
    SmackClose(m_smackHandle);
    m_smackHandle = 0;
    if (m_rezBuffer) {
        RezFree_call(m_rezBuffer);
        m_rezBuffer = 0;
    }
    m_streamOpen = 0;
    return 1;
}

// CMoviePlayer::Frame (0x17caa0) - the per-frame renderer CMoviePlayer::Pump drives, re-homed
// from the ApiCaller stubs (was MoviePlayer_17caa0::RenderFrame; class identity proven -
// Pump calls it on this=CSmackWin, and CSmackWin already declared Frame @0x17caa0).
// Locks the DDraw surface (retrying on DDERR_SURFACELOST), decodes the current Smacker
// frame into it, blits the changed region(s), then advances to the next frame (0 once
// the last frame has played). m_24 is now the real IDirectDrawSurface (the former
// SmkBuf/manual-vtable views are folded into it).
RVA(0x0017caa0, 0x13b)
i32 CMoviePlayer::Frame() {
    if (m_smackHandle->NewPalette && m_520 == 8) {
        SnapshotPalette();
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
            BlitDirty(
                m_smackHandle->LastRectx,
                m_smackHandle->LastRecty,
                m_smackHandle->LastRectw,
                m_smackHandle->LastRecth
            );
        }
    } else {
        BlitDirty(0, 0, m_smackHandle->Width, m_smackHandle->Height);
    }
    Smack* s = m_smackHandle;
    if (s->FrameNum == s->Frames - 1) {
        return 0;
    }
    SmackNextFrame(s);
    return 1;
}

// __thiscall(loops): play the whole m_868c clip playlist `loops` times. Each pass
// walks the CMovieClip* array (m_pData/m_nSize): opens each clip (OpenLo when its
// m_openArg is 0, else the full Open), pumps it, and - when a command surface
// (m_command) is set - flips it (Blt DDFX) then color-fills it black (Blt COLORFILL),
// closing the stream after each. A null clip source or any Open/Pump early-out
// returns before completion; a full run returns 0x11111111.
RVA(0x0017d720, 0x188)
i32 CMoviePlayer::PlayList(i32 loops) {
    if (!m_active || loops < -1 || loops == 0) {
        return 0;
    }
    i32 iter = 1;
    do {
        for (i32 i = 0; i < m_clipCount; i++) {
            CMovieClip* clip = m_868c.m_pData[i];
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
            CMovieClip* c2 = m_868c.m_pData[i];
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
