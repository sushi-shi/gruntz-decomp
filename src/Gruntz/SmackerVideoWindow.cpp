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
#include <Win32.h> // HWND, HMENU, GetSystemMetrics
#include <Ints.h>
#include <rva.h>

#include <Gruntz/CWnd.h>

// LPCTSTR AFXAPI AfxRegisterWndClass(UINT, HCURSOR=0, HBRUSH=0, HICON=0). __stdcall.
extern "C" const char* __stdcall
AfxRegisterWndClass(u32 style, void* cur, void* brush, void* icon); // 0x1bc09d

// CString == its 4-byte m_pchData; passing it where LPCTSTR is wanted yields that ptr.
struct CString {
    char* m_pchData;
    CString(const char* s); // 0x1b9d4c
    ~CString();             // 0x1b9cde
    operator const char*() const {
        return m_pchData;
    }
};

// The created window is the shared MFC CWnd (0x3c bytes, m_hWnd at +0x1c; see
// <Gruntz/CWnd.h>). Ctor/CreateEx/SetFocus are external (the CObject vtable is
// stamped by the real ctor).

// The video window seen through its manual playback vtable during teardown: slot 1
// (+0x04) is the scalar-deleting destroy, slot 24 (+0x60) the finalize hook.
struct CursSink {
    virtual void v0();
    virtual void Destroy(i32 del); // slot 1 == vtable +0x04
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void v17();
    virtual void v18();
    virtual void v19();
    virtual void v20();
    virtual void v21();
    virtual void v22();
    virtual void v23();
    virtual void Finish(); // slot 24 == vtable +0x60
};

// Smacker imports (IAT) reached during open/pump/close.
extern "C" __declspec(dllimport) void __stdcall SmackSoundUseDirectSound(void* ds);
extern "C" __declspec(dllimport) i32 __stdcall SmackOpen(i32 src, u32 flags, i32 buf);
extern "C" __declspec(dllimport) i32 __stdcall SmackWait(i32 smk);
extern "C" __declspec(dllimport) void __stdcall SmackSoundOnOff(i32 smk, i32 on);
extern "C" __declspec(dllimport) void __stdcall SmackGoto(i32 smk, u32 frame);
extern "C" __declspec(dllimport) u32 __stdcall SmackClose(i32 smk);
// The Rez allocator's free (RVA 0x1b9b82).
extern "C" void RezFree_call(void* p);

// A releasable sub-buffer reached via manual vtable dispatch (slot +0x08).
struct SmkBufVtbl;
struct SmkBuf {
    SmkBufVtbl* m_vptr;
};
struct SmkBufVtbl {
    void* s0[2];
    void(__stdcall* Release)(SmkBuf*); // +0x08
};
// The embedded sub-player whose Shutdown() lives at RVA 0x17b570.
struct SmackSub {
    void Shutdown(); // RVA 0x17b570
};

struct CSmackWin {
    i32 m_0; // +0x00
    i32 m_4; // +0x04 active flag
    i32 m_8; // +0x08 stream-open flag
    char _0c[0x10 - 0xc];
    i32 m_10; // +0x10 Smacker handle
    char _14[0x1c - 0x14];
    i32 m_1c; // +0x1c pending command
    char _20[0x24 - 0x20];
    SmkBuf* m_24; // +0x24
    SmkBuf* m_28; // +0x28
    char _2c[0x508 - 0x2c];
    void* m_508; // +0x508 DirectSound
    char _50c[0x514 - 0x50c];
    i32 m_514; // +0x514
    char _518[0x534 - 0x518];
    void* m_534;    // +0x534 Rez buffer
    i32 m_538;      // +0x538
    CWnd* m_53c;    // +0x53c  the video window
    SmackSub m_540; // +0x540 embedded sub-player
    char _544[0x86a0 - 0x544];
    i32 m_86a0;                                             // +0x86a0 loop counter
    int Init(HWND h, i32 a0, i32 a1);                       // 0x17c040
    int CreateVideoWindow(i32 a0, i32 a1);                  // 0x17c2a0
    void Teardown();                                        // 0x17c510
    i32 OpenLo(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5); // 0x17c570
    i32 OpenHi(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5); // 0x17c630
    i32 Pump(i32 flags, i32 count);                         // 0x17c790
    i32 Advance(i32 cmd, i32 loops);                        // 0x17c8e0
    i32 CloseSmacker();                                     // 0x17c9b0
    i32 Begin(i32 a2, i32 useDS, i32 a4, i32 a5);           // 0x17cfc0 (external)
    i32 Frame();                                            // 0x17caa0 (external)
    void Free17d6b0();                                      // 0x17d6b0
    void Free17cc80();                                      // 0x17cc80
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
int CSmackWin::CreateVideoWindow(i32 a0, i32 a1) {
    CString cls(AfxRegisterWndClass(3, 0, 0, 0));
    if (m_53c != 0) {
        return 0;
    }
    m_53c = new CWnd;
    if (!m_53c->CreateEx(
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
    m_53c->SetFocus();
    HWND h = m_53c ? m_53c->m_hWnd : 0;
    return Init(h, a0, a1);
}

// __thiscall(): tear the playback object down and restore the cursor. Re-homed
// from ApiCallers (placeholder CursHost_17c510): its m_53c (video window at +0x53c)
// is the same CWnd CreateVideoWindow above `new`s, so this is the same CSmackWin.
RVA(0x0017c510, 0x5e)
void CSmackWin::Teardown() {
    if (!m_4) {
        return;
    }
    CloseSmacker();
    Free17d6b0();
    m_0 = 0;
    m_4 = 0;
    Free17cc80();
    if (m_53c) {
        ((CursSink*)m_53c)->Finish();
        if (m_53c) {
            ((CursSink*)m_53c)->Destroy(1);
        }
        m_53c = 0;
    }
    ShowCursor(1);
}

// __thiscall(src,a2,useDS,a4,a5): open a Smacker stream (0xfe000 flags, plus
// 0x100000 when DirectSound is requested), begin playback, roll back on failure.
RVA(0x0017c570, 0xc0)
i32 CSmackWin::OpenLo(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5) {
    if (!m_4) {
        return 0;
    }
    SmackSoundUseDirectSound(m_508);
    m_514 = a2;
    u32 flags;
    if (useDS == 1) {
        m_538 = useDS;
        flags = 0x100000;
    } else {
        m_538 = 0;
        flags = 0;
    }
    flags |= 0xfe000;
    m_10 = SmackOpen(src, flags, -1);
    if (!m_10) {
        return 0;
    }
    m_8 = 1;
    i32 r = Begin(a2, useDS, a4, a5);
    if (r) {
        return r;
    }
    if (m_24) {
        m_24->m_vptr->Release(m_24);
        m_24 = 0;
    }
    if (m_28) {
        m_28->m_vptr->Release(m_28);
        m_28 = 0;
    }
    CloseSmacker();
    return r;
}

// __thiscall(src,a2,useDS,a4,a5): same as OpenLo but with the 0xff000 flag set.
RVA(0x0017c630, 0xc0)
i32 CSmackWin::OpenHi(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5) {
    if (!m_4) {
        return 0;
    }
    SmackSoundUseDirectSound(m_508);
    m_514 = a2;
    u32 flags;
    if (useDS == 1) {
        flags = 0x100000;
        m_538 = useDS;
    } else {
        m_538 = 0;
        flags = 0;
    }
    flags |= 0xff000;
    m_10 = SmackOpen(src, flags, -1);
    if (!m_10) {
        return 0;
    }
    m_8 = 1;
    i32 r = Begin(a2, useDS, a4, a5);
    if (r) {
        return r;
    }
    if (m_24) {
        m_24->m_vptr->Release(m_24);
        m_24 = 0;
    }
    if (m_28) {
        m_28->m_vptr->Release(m_28);
        m_28 = 0;
    }
    CloseSmacker();
    return r;
}

// __thiscall(flags, count): pump the Win32 queue while a Smacker movie plays; abort
// with 1/0x100 on a key/mouse event the abort flags select, else render the next
// frame and re-loop until `count` plays elapse (count==-1 loops forever).
RVA(0x0017c790, 0x14a)
i32 CSmackWin::Pump(i32 flags, i32 count) {
    if (!m_4 || count < -1 || count == 0) {
        return 0;
    }
    m_86a0 = 1;
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
            if (SmackWait(m_10)) {
                continue;
            }
            if (Frame()) {
                continue;
            }
            if (count != -1 && ++m_86a0 > count) {
                return 0x11111111;
            }
            SmackSoundOnOff(m_10, 0);
            SmackGoto(m_10, 1);
            SmackSoundOnOff(m_10, 1);
        }
    }
}

// __thiscall(cmd, loops): wait for the stream, render a frame, and on EOF loop
// back to the start until `loops` is exhausted (loops==-1 loops forever).
RVA(0x0017c8e0, 0xca)
i32 CSmackWin::Advance(i32 cmd, i32 loops) {
    if (!cmd || !m_4 || loops < -1 || loops == 0) {
        return 0;
    }
    i32 result = 1;
    if (m_86a0 == 0) {
        m_86a0 = result;
    }
    if (SmackWait(m_10) == 0) {
        i32 saved = m_1c;
        m_1c = cmd;
        result = Frame();
        if (result == 0) {
            if (loops == -1 || ++m_86a0 <= loops) {
                SmackSoundOnOff(m_10, 0);
                SmackGoto(m_10, 1);
                SmackSoundOnOff(m_10, 1);
                result = 1;
            }
        }
        m_1c = saved;
    }
    if (result == 0) {
        m_86a0 = 0;
    }
    return result;
}

// __thiscall: shut the sub-player, close the Smacker stream, free buffers.
RVA(0x0017c9b0, 0x5b)
i32 CSmackWin::CloseSmacker() {
    if (!m_8) {
        return 0;
    }
    m_540.Shutdown();
    if (!m_10) {
        return 0;
    }
    SmackClose(m_10);
    m_10 = 0;
    if (m_534) {
        RezFree_call(m_534);
        m_534 = 0;
    }
    m_8 = 0;
    return 1;
}
SIZE_UNKNOWN(CSmackWin);
