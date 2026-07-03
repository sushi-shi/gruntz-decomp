// AilMidiSeq.cpp - the Miles Sound System (AIL) MIDI/XMIDI sequence subsystem,
// re-homed out of the src/Stub/ApiCallers.cpp winapi grab-bag. This is the
// "sibling AIL TU" referenced by include/Dsndmgr/CGruntzSoundZ.h: the AIL
// sequence player (0x138dd0-0x139030), its per-bank record init (0x138c20), the
// XMIDI master-volume helpers (0x138950/0x1389c0), the driver bring-up/teardown
// (0x138490/0x1384f0), and the low-level MIDI-device DLL teardown (0xf8e20).
//
// The placeholder classes (AilSeq / AilSeq_138c20 / AilHost_138490 / AilMgr_1384f0)
// map onto CGruntzSoundInnerZ / CGruntzSoundZ; folding them onto that real
// polymorphic class (VTBL 0x1ef700) is deferred to avoid double-emitting its
// vtable. Field names are placeholders; offsets + emitted bytes are load-bearing.
#include <Win32.h>
#include <rva.h>
#include <string.h>

// Miles Sound System (AIL) imports - reached through the IAT (ff 15 [__imp]).
extern "C" {
    __declspec(dllimport) i32 __stdcall AIL_set_XMIDI_master_volume(i32 driver, i32 volume);
    __declspec(dllimport) i32 __stdcall AIL_start_sequence(i32 seq);
    __declspec(dllimport) i32 __stdcall AIL_set_sequence_loop_count(i32 seq, i32 count);
    __declspec(dllimport) i32 __stdcall AIL_resume_sequence(i32 seq);
    __declspec(dllimport) void __stdcall AIL_startup();
    __declspec(dllimport) i32 __stdcall AIL_midiOutOpen(i32* driver, i32 dunno, i32 devid);
    __declspec(dllimport) i32 __stdcall AIL_XMIDI_master_volume(i32 driver);
    __declspec(dllimport) void __stdcall AIL_end_sequence(i32 seq);
    __declspec(dllimport) void __stdcall AIL_set_sequence_tempo(i32 seq, i32 tempo, i32 ms);
    __declspec(dllimport) void __stdcall AIL_shutdown();
    __declspec(dllimport) void __stdcall AIL_stop_sequence(i32 seq);
    __declspec(dllimport) void __stdcall AIL_set_sequence_volume(i32 seq, i32 volume, i32 ms);
    __declspec(dllimport) void __stdcall AIL_release_sequence_handle(i32 seq);
    __declspec(dllimport) i32 __stdcall AIL_allocate_sequence_handle(i32 driver);
    __declspec(dllimport) i32 __stdcall AIL_init_sequence(i32 seq, void* xmidi, i32 seqNum);
}

// The AIL MIDI driver handle (DAT_00653c5c), 0 when no driver is open.
DATA(0x00253c5c)
extern i32 g_ailMidiDriver;
// Monotonic counter naming auto-generated MIDI sequences ("MIDI%i", DAT_00653c60).
DATA(0x00253c60)
extern i32 g_midiSeqCounter;
// Cached AIL driver handle passed to AIL_set_* (DAT_00653c64).
DATA(0x00253c64)
extern i32 g_ailDriver64;

// The Rez heap allocator (_RezAlloc, defined in EngineExternFns.cpp).
extern "C" void* RezAlloc(u32 size);
extern "C" void RezFree_call(void* p);                      // RVA 0x1b9b82 (cdecl)
i32 __cdecl Format_11f890(char* buf, const char* fmt, ...); // RVA 0x11f890

namespace AilMidi {
    // --- 0xf8e20: the low-level MIDI-device DLL teardown ---
    struct MidiDrv_0f8e20 {
        char m_pad0[0x14];
        void(__cdecl* m_14)(i16); // +0x14 function pointer member
    };
    // The MIDI-device DLL state (low-level MIDI-out driver).
    DATA(0x0024e0b8)
    extern i32 g_midiOpen_64e0b8;
    DATA(0x0024e0b0)
    extern MidiDrv_0f8e20* g_midiDrv_64e0b0;
    DATA(0x0024e0a4)
    extern i16 g_midiPort_64e0a4;
    DATA(0x0024dd28)
    extern i16 g_midiDev_64dd28;
    DATA(0x0024e0a8)
    extern HMODULE g_midiLib_64e0a8;
    void __cdecl MidiShutdown_3382(); // RVA 0x3382

    // __cdecl(): tear the MIDI driver down and free its DLL.
    RVA(0x000f8e20, 0x56)
    void winapi_0f8e20_FreeLibrary() {
        if (g_midiOpen_64e0b8 && g_midiDrv_64e0b0 && g_midiPort_64e0a4) {
            MidiShutdown_3382();
            g_midiDrv_64e0b0->m_14(g_midiDev_64dd28);
            FreeLibrary(g_midiLib_64e0a8);
            g_midiLib_64e0a8 = 0;
            g_midiOpen_64e0b8 = 0;
        }
    }

    // __thiscall(driver, seq, skipInit): record the AIL handles, optionally start
    // the AIL MIDI driver. m_28 = whether the driver is usable.
    struct AilHost_138490 {
        char m_pad0[0x1c];
        i32 m_1c; // +0x1c
        i32 m_20; // +0x20
        i32 m_24; // +0x24
        i32 m_28; // +0x28
        i32 Init(i32 driver, i32 seq, i32 skipInit);
    };
    RVA(0x00138490, 0x5e)
    i32 AilHost_138490::Init(i32 driver, i32 seq, i32 skipInit) {
        m_24 = driver;
        m_20 = seq;
        m_1c = 0;
        m_28 = 1;
        g_ailDriver64 = driver;
        if (!skipInit) {
            AIL_startup();
            if (AIL_midiOutOpen(&g_ailMidiDriver, 0, -1) != 0 || g_ailMidiDriver == 0) {
                m_28 = 0;
            }
        }
        return 1;
    }

    // A polymorphic sub whose slot 12 (vtable +0x30) tears down its sequence.
    struct AilSlot_1384f0 {
        virtual void v0();
        virtual void v1();
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
        virtual void Release(); // slot 12 == vtable +0x30
    };
    struct AilMgr_1384f0 {
        char m_pad0[0x1c];
        AilSlot_1384f0* m_1c; // +0x1c
        i32 m_20;             // +0x20
        void Shutdown();
        void Reset138530(); // thiscall, RVA 0x138530
    };
    // __thiscall(): reset, release the active sequence, reset again, shut AIL down.
    RVA(0x001384f0, 0x3b)
    void AilMgr_1384f0::Shutdown() {
        Reset138530();
        if (m_1c) {
            m_1c->Release();
        }
        Reset138530();
        m_20 = 0;
        m_1c = 0;
        g_ailMidiDriver = 0;
        AIL_shutdown();
    }

    // __stdcall(volume 0..100): scale to 0..127 and push to the XMIDI driver.
    RVA(0x00138950, 0x70)
    i32 __stdcall thirdparty_138950_AIL_set_XMIDI_master_volume_8(i32 volume) {
        i32 scaled;
        if (!g_ailMidiDriver) {
            return 0;
        }
        if (volume <= 0) {
            scaled = 0;
        } else if (volume >= 100) {
            scaled = 0x7f;
        } else {
            scaled = volume * 127 / 100;
        }
        AIL_set_XMIDI_master_volume(g_ailMidiDriver, scaled);
        return 1;
    }

    // __cdecl(): read the XMIDI master volume and rescale 0..127 -> 0..100.
    RVA(0x001389c0, 0x47)
    i32 thirdparty_1389c0_AIL_XMIDI_master_volume_4() {
        if (!g_ailMidiDriver) {
            return 0x64;
        }
        i32 v = AIL_XMIDI_master_volume(g_ailMidiDriver);
        if (v <= 0) {
            return 0;
        }
        if (v >= 0x7f) {
            return 0x64;
        }
        return v * 100 / 127;
    }

    // An owned XMIDI sequence: copies the song bytes into a Rez buffer, names it,
    // and allocates + initialises an AIL sequence handle against the MIDI driver.
    struct AilSeq_138c20 {
        char m_pad0[4];
        char m_name[0x48 - 4]; // +0x04 sequence name buffer
        i32 m_48;              // +0x48
        char m_pad4c[0x50 - 0x4c];
        i32 m_50;   // +0x50
        i32 m_54;   // +0x54
        i32 m_58;   // +0x58 AIL sequence handle
        void* m_5c; // +0x5c owned song bytes
        i32 Init(void* data, u32 size, char* name);
    };
    // __thiscall(data, size, name): seed a sequence record from `size` bytes of
    // XMIDI at `data`; auto-name "MIDI%i" when `name` is null.
    RVA(0x00138c20, 0x122)
    i32 AilSeq_138c20::Init(void* data, u32 size, char* name) {
        if (!data) {
            return 0;
        }
        if (size < 4) {
            return 0;
        }
        if (!g_ailMidiDriver) {
            return 0;
        }
        ++g_midiSeqCounter;
        m_48 = 0;
        m_54 = 100;
        m_50 = 100;
        if (name) {
            strcpy(m_name, name);
        } else {
            Format_11f890(m_name, "MIDI%i", g_midiSeqCounter);
        }
        if (!m_5c) {
            m_5c = RezAlloc(size);
            if (!m_5c) {
                return 0;
            }
            memcpy(m_5c, data, size);
        }
        m_58 = AIL_allocate_sequence_handle(g_ailMidiDriver);
        if (!m_58) {
            return 0;
        }
        if (AIL_init_sequence(m_58, m_5c, 0) == 0) {
            AIL_release_sequence_handle(m_58);
            m_58 = 0;
            return 0;
        }
        return 1;
    }

    // AIL sequence player. The virtual at slot 8 (vtable +0x20) gates playback;
    // the sequence handle lives at m_58, loop/cursor state at m_44/m_48/m_4c.
    struct AilSeq {
        // vptr at +0x00 (compiler-managed). Eight leading virtuals put CanPlay at
        // vtable offset 0x20; slot 12 (+0x30) is the teardown hook.
        virtual void v0();
        virtual void v1();
        virtual void v2();
        virtual void v3();
        virtual void v4();
        virtual void v5();
        virtual void v6();
        virtual void v7();
        virtual i32 CanPlay(); // slot 8 == vtable +0x20
        virtual void v9();
        virtual void v10();
        virtual void v11();
        virtual void Teardown(); // slot 12 == vtable +0x30
        char m_pad4[0x44 - 4];
        i32 m_44; // +0x44
        i32 m_48; // +0x48
        i32 m_4c; // +0x4c
        i32 m_50; // +0x50
        i32 m_54; // +0x54
        i32 m_58; // +0x58
        i32 m_5c; // +0x5c
        i32 Play(i32 cursor, i32 loop);
        i32 Resume(i32 restart);
        i32 SetLoop(i32 loop);
        i32 ResumeGate(); // the m_138f60 helper
        i32 Stop();
        i32 SetTempo(i32 tempo, i32 unused);
        void ReleaseHandle();
        i32 Pause();
        i32 SetVolume(i32 volume, i32 ms);
    };

    // __thiscall(): tear down, release the AIL sequence handle + Rez buffer.
    RVA(0x00138dd0, 0x36)
    void AilSeq::ReleaseHandle() {
        Teardown();
        if (m_58) {
            AIL_release_sequence_handle(m_58);
            m_58 = 0;
        }
        if (m_5c) {
            RezFree_call((void*)m_5c);
            m_5c = 0;
        }
    }

    // __thiscall(cursor, loop): start the sequence; if looping, clear loop count.
    RVA(0x00138e10, 0x4a)
    i32 AilSeq::Play(i32 cursor, i32 loop) {
        if (!CanPlay()) {
            return 0;
        }
        m_4c = cursor;
        m_48 = loop;
        AIL_start_sequence(m_58);
        if (loop) {
            AIL_set_sequence_loop_count(m_58, 0);
        }
        m_44 = 0;
        return 1;
    }

    // __thiscall(): if playable, end the sequence and clear the paused flag.
    RVA(0x00138e60, 0x26)
    i32 AilSeq::Stop() {
        if (!CanPlay()) {
            return 0;
        }
        AIL_end_sequence(m_58);
        m_44 = 0;
        return 1;
    }

    // __thiscall(): pause the sequence the first time, counting nested pauses.
    RVA(0x00138e90, 0x3a)
    i32 AilSeq::Pause() {
        if (!CanPlay()) {
            return 0;
        }
        if (!ResumeGate()) {
            return 0;
        }
        if (m_44 == 0) {
            AIL_stop_sequence(m_58);
        }
        m_44++;
        return 1;
    }

    // __thiscall(restart): count down the resume delay and re-issue the resume.
    RVA(0x00138ed0, 0x4f)
    i32 AilSeq::Resume(i32 restart) {
        if (!CanPlay()) {
            return 0;
        }
        if (ResumeGate()) {
            return 1;
        }
        if (m_44 > 0) {
            m_44--;
            if (restart) {
                m_44 = 0;
            }
            if (m_44 <= 0) {
                AIL_resume_sequence(m_58);
            }
        }
        return 1;
    }

    // __thiscall(tempo, ms): if playable, set the sequence tempo and cache it.
    RVA(0x00138f90, 0x32)
    i32 AilSeq::SetTempo(i32 tempo, i32 ms) {
        if (!CanPlay()) {
            return 0;
        }
        AIL_set_sequence_tempo(m_58, tempo, ms);
        m_54 = tempo;
        return 1;
    }

    // __thiscall(volume 0..100, ms): scale to 0..127 and set the sequence volume.
    RVA(0x00138fd0, 0x5e)
    i32 AilSeq::SetVolume(i32 volume, i32 ms) {
        if (!CanPlay()) {
            return 0;
        }
        i32 scaled;
        if (volume <= 0) {
            scaled = 0;
        } else if (volume >= 100) {
            scaled = 0x7f;
        } else {
            scaled = volume * 127 / 100;
        }
        AIL_set_sequence_volume(m_58, scaled, ms);
        m_50 = volume;
        return 1;
    }

    // __thiscall(loop): update the cached loop flag, re-arming the driver count.
    RVA(0x00139030, 0x4c)
    i32 AilSeq::SetLoop(i32 loop) {
        if (!CanPlay()) {
            return 0;
        }
        if (m_48 != loop) {
            m_48 = loop;
            if (loop) {
                AIL_set_sequence_loop_count(m_58, 0);
            } else {
                AIL_set_sequence_loop_count(m_58, 1);
            }
        }
        return 1;
    }
} // namespace AilMidi
