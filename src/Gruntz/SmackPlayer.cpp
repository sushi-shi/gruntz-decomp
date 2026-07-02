#include <Win32.h>

#include <rva.h>

// Smacker (Bink/RAD) video playback cluster (RVA 0x17c510..0x17cd90), re-homed out
// of the artificial src/Stub/ApiCallers.cpp aggregate. GAME code: __thiscall wrappers
// over the Smacker C imports + a DirectDraw surface blit path. No module-scope globals.

namespace ApiCallerStubs {
    // A polymorphic sub: slot 24 (+0x60) finalizes, slot 1 (+0x04) deletes it.
    struct CursSink_17c510 {
        virtual void v0();
        virtual void Destroy(i32 del); // slot 1 == vtable +0x04
        // slots 2..23 elided as a gap so Finish lands at +0x60
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
    struct CursHost_17c510 {
        i32 m_0; // +0x00
        i32 m_4; // +0x04 active flag
        char m_pad8[0x53c - 8];
        CursSink_17c510* m_53c; // +0x53c
        void Teardown();
        void CloseSmacker(); // RVA 0x17c9b0
        void Free17d6b0();   // RVA 0x17d6b0
        void Free17cc80();   // RVA 0x17cc80
    };
    // __thiscall(): tear the playback object down and restore the cursor.
    RVA(0x0017c510, 0x5e)
    void CursHost_17c510::Teardown() {
        if (!m_4) {
            return;
        }
        CloseSmacker();
        Free17d6b0();
        m_0 = 0;
        m_4 = 0;
        Free17cc80();
        if (m_53c) {
            m_53c->Finish();
            if (m_53c) {
                m_53c->Destroy(1);
            }
            m_53c = 0;
        }
        ShowCursor(1);
    }

    // Smacker imports (IAT): route audio through DirectSound, then open the stream.
    extern "C" __declspec(dllimport) void __stdcall SmackSoundUseDirectSound(void* ds);
    extern "C" __declspec(dllimport) i32 __stdcall SmackOpen(i32 src, u32 flags, i32 buf);
    // A releasable sub-buffer reached via manual vtable dispatch (slot +0x08).
    struct SmkBufVtbl_17c570;
    struct SmkBuf_17c570 {
        SmkBufVtbl_17c570* m_vptr;
    };
    struct SmkBufVtbl_17c570 {
        void* s0[2];
        void(__stdcall* Release)(SmkBuf_17c570*); // +0x08
    };
    struct SmkPlayer_17c570 {
        char m_pad0[4];
        i32 m_4; // +0x04 active flag
        i32 m_8; // +0x08
        char m_pad0c[0x10 - 0xc];
        i32 m_10; // +0x10 Smacker handle
        char m_pad14[0x24 - 0x14];
        SmkBuf_17c570* m_24; // +0x24
        SmkBuf_17c570* m_28; // +0x28
        char m_pad2c[0x508 - 0x2c];
        void* m_508; // +0x508 DirectSound
        char m_pad50c[0x514 - 0x50c];
        i32 m_514; // +0x514
        char m_pad518[0x538 - 0x518];
        i32 m_538;                                    // +0x538
        i32 Begin(i32 a2, i32 useDS, i32 a4, i32 a5); // RVA 0x17cfc0
        void CloseSmacker();                          // RVA 0x17c9b0
        i32 OpenLo(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5);
        i32 OpenHi(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5);
    };
    // __thiscall(src,a2,useDS,a4,a5): open a Smacker stream (0xfe000 flags, plus
    // 0x100000 when DirectSound is requested), begin playback, roll back on failure.
    RVA(0x0017c570, 0xc0)
    i32 SmkPlayer_17c570::OpenLo(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5) {
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
    i32 SmkPlayer_17c570::OpenHi(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5) {
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

    // Smacker playback advance: the per-frame "step / loop" driver.
    extern "C" __declspec(dllimport) i32 __stdcall SmackWait(i32 smk);
    extern "C" __declspec(dllimport) void __stdcall SmackSoundOnOff(i32 smk, i32 on);
    extern "C" __declspec(dllimport) void __stdcall SmackGoto(i32 smk, u32 frame);
    struct Movie_17c8e0 {
        char m_pad0[4];
        i32 m_4; // +0x04 active flag
        char m_pad8[0x10 - 8];
        i32 m_10; // +0x10 Smacker handle
        char m_pad14[0x1c - 0x14];
        i32 m_1c; // +0x1c pending command
        char m_pad20[0x86a0 - 0x20];
        i32 m_86a0;                     // +0x86a0 loop counter
        i32 Frame();                    // RVA 0x17caa0 (renders the next frame)
        i32 Pump(i32 flags, i32 count); // RVA 0x17c790
        i32 Advance(i32 cmd, i32 loops);
    };

    // __thiscall(flags, count): pump the Win32 queue while a Smacker movie plays; abort
    // with 1/0x100 on a key/mouse event the abort flags select, else render the next
    // frame and re-loop until `count` plays elapse (count==-1 loops forever).
    RVA(0x0017c790, 0x14a)
    i32 Movie_17c8e0::Pump(i32 flags, i32 count) {
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
    i32 Movie_17c8e0::Advance(i32 cmd, i32 loops) {
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

    // Smacker import (IAT) + the Rez allocator's free (RVA 0x1b9b82).
    extern "C" __declspec(dllimport) u32 __stdcall SmackClose(i32 smk);
    extern "C" void RezFree_call(void* p); // RVA 0x1b9b82 (cdecl)
    // The embedded sub-player whose Shutdown() lives at RVA 0x17b570.
    struct SmackSub_17c9b0 {
        void Shutdown(); // RVA 0x17b570
    };
    struct SmackHost_17c9b0 {
        char m_pad0[8];
        i32 m_8; // +0x08 active flag
        char m_pad0c[0x10 - 0xc];
        i32 m_10; // +0x10 Smacker handle
        char m_pad14[0x534 - 0x14];
        void* m_534; // +0x534 Rez buffer
        char m_pad538[0x540 - 0x538];
        SmackSub_17c9b0 m_540; // +0x540 sub-player
        i32 Close();
    };
    // __thiscall: shut the sub-player, close the Smacker stream, free buffers.
    RVA(0x0017c9b0, 0x5b)
    i32 SmackHost_17c9b0::Close() {
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

    extern "C" __declspec(dllimport) void __stdcall
    SmackToBuffer(void* smk, i32 left, i32 top, i32 pitch, i32 height, void* buf, i32 flags);
    extern "C" __declspec(dllimport) void __stdcall SmackDoFrame(void* smk);
    extern "C" __declspec(dllimport) i32 __stdcall SmackToBufferRect(void* smk, i32 flags);
    extern "C" __declspec(dllimport) void __stdcall SmackNextFrame(void* smk);
    // The DirectDraw surface the frame is locked/blitted into (manual vtable).
    struct DDSurf_17caa0;
    struct DDSurfVtbl_17caa0 {
        void* s0[25];                                                   // +0x00..+0x60
        i32(__stdcall* Lock)(DDSurf_17caa0*, void*, void*, u32, void*); // +0x64
        void* s26;                                                      // +0x68
        i32(__stdcall* Restore)(DDSurf_17caa0*);                        // +0x6c
        void* s28[4];                                                   // +0x70..+0x7c
        i32(__stdcall* Unlock)(DDSurf_17caa0*, void*);                  // +0x80
    };
    struct DDSurf_17caa0 {
        DDSurfVtbl_17caa0* vptr;
    };
    // The decoded Smacker stream header.
    struct Smack_17caa0 {
        char m_pad0[4];
        i32 m_4; // +0x04 width
        i32 m_8; // +0x08 height
        i32 m_c; // +0x0c frame count
        char m_pad10[0x68 - 0x10];
        i32 m_68; // +0x68
        char m_pad6c[0x374 - 0x6c];
        i32 m_374; // +0x374 current frame
        char m_pad378[0x380 - 0x378];
        i32 m_380; // +0x380 dirty-rect left
        i32 m_384; // +0x384 dirty-rect top
        i32 m_388; // +0x388 dirty-rect right
        i32 m_38c; // +0x38c dirty-rect bottom
    };
    struct MoviePlayer_17caa0 {
        char m_pad0[0x10];
        Smack_17caa0* m_10; // +0x10
        char m_pad14[0x24 - 0x14];
        DDSurf_17caa0* m_24; // +0x24
        char m_pad28[0x9c - 0x28];
        char m_desc[0xac - 0x9c]; // +0x9c DDSURFACEDESC head
        i32 m_ac;                 // +0xac desc.lPitch
        char m_padb0[0xc0 - 0xb0];
        void* m_c0; // +0xc0 desc.lpSurface
        char m_padc4[0x50c - 0xc4];
        i32 m_50c; // +0x50c
        i32 m_510; // +0x510 flags
        i32 m_514; // +0x514 full-frame flag
        char m_pad518[0x520 - 0x518];
        i32 m_520;                          // +0x520
        void Sub17ca10();                   // RVA 0x17ca10
        void Sub17cdf0(i32, i32, i32, i32); // RVA 0x17cdf0 (blit dirty rect)
        i32 RenderFrame();
    };
    // __thiscall(): lock the surface, decode the current frame into it, blit the
    // changed region, then advance to the next frame (0 once the last frame plays).
    RVA(0x0017caa0, 0x13b)
    i32 MoviePlayer_17caa0::RenderFrame() {
        if (m_10->m_68 && m_520 == 8) {
            Sub17ca10();
        }
        i32 hr = m_24->vptr->Lock(m_24, 0, m_desc, 1, 0);
        while (hr == (i32)0x887601c2) {
            if (m_24->vptr->Restore(m_24) != 0) {
                goto afterLock;
            }
            hr = m_24->vptr->Lock(m_24, 0, m_desc, 1, 0);
        }
        if (hr == 0) {
            SmackToBuffer(m_10, 0, 0, m_ac, m_10->m_8, m_c0, m_510);
            SmackDoFrame(m_10);
            m_50c = 1;
            m_24->vptr->Unlock(m_24, m_c0);
        }
    afterLock:
        if (m_514 != 1) {
            while (SmackToBufferRect(m_10, 0) != 0) {
                Sub17cdf0(m_10->m_380, m_10->m_384, m_10->m_388, m_10->m_38c);
            }
        } else {
            Sub17cdf0(0, 0, m_10->m_4, m_10->m_8);
        }
        Smack_17caa0* s = m_10;
        if (s->m_374 == s->m_c - 1) {
            return 0;
        }
        SmackNextFrame(s);
        return 1;
    }

    struct PalCache_17cd90 {
        char m_pad0[0x108];
        PALETTEENTRY m_108[0x100]; // +0x108
        void Snapshot(HWND hWnd);
    };
    // __thiscall(hWnd): read the system palette, then blank every entry to a
    // reserved black so a remap can be rebuilt against it.
    RVA(0x0017cd90, 0x58)
    void PalCache_17cd90::Snapshot(HWND hWnd) {
        HDC hdc = GetDC(hWnd);
        GetSystemPaletteEntries(hdc, 0, 0x100, m_108);
        for (i32 i = 0; i < 0x100; i++) {
            m_108[i].peRed = 0;
            m_108[i].peBlue = 0;
            m_108[i].peGreen = 0;
            m_108[i].peFlags = 4;
        }
        ReleaseDC(hWnd, hdc);
    }

} // namespace ApiCallerStubs
