#include <Dsndmgr/GruntzSoundZ.h>
#include <rva.h>

#include <mss.h>    // the genuine Miles Sound System (MSS32.DLL) AIL/XMIDI SDK header
#include <stdio.h>  // sprintf - the "MIDI%i" auto-name (0x11f890, _sprintf)
#include <string.h> // strcpy/memcpy (inline rep movs / repne scas under /O2)

DATA(0x00253c5c)
HMDIDRIVER g_ailMidiDriver = 0; // 0x653c5c  AIL/digital MIDI driver handle (0 = none open)
DATA(0x00253c60)
i32 g_midiSeqCounter = 0; // 0x653c60  monotonic auto-name counter ("MIDI%i")
DATA(0x00253c64)
HMDIDRIVER g_ailDriver64 = 0; // 0x653c64  cached driver handle passed to Init

// ---------------------------------------------------------------------------
// ~CGruntzSoundZ destructor: stop/flush everything via Shutdown, then the m_map
// member ~CMapStringToOb fires from the destructor epilogue. The destructible
// member forces the /GX EH state machine (state 0 across Shutdown, -1 after).
// Non-polymorphic containment (see the header note): retail emits NO own-vptr
// restamp here, which the old derived model could not reproduce (was @early-stop
// at ~91.6% on the extra ??_7CGruntzSoundZ entry stamp).
RVA(0x00086040, 0x49)
CGruntzSoundZ::~CGruntzSoundZ() {
    Shutdown();
}

RVA(0x00138490, 0x5e)
i32 CGruntzSoundZ::Init(i32 mdiHandle, i32 digHandle, i32 skipInit) {
    m_mdiHandle = mdiHandle;
    m_digHandle = digHandle;
    m_pCurrent = 0;
    m_enabled = 1;
    g_ailDriver64 = reinterpret_cast<HMDIDRIVER>(mdiHandle);
    if (skipInit == 0) {
        AIL_startup();
        if (AIL_midiOutOpen(&g_ailMidiDriver, 0, -1) != 0 || g_ailMidiDriver == 0) {
            m_enabled = 0;
        }
    }
    return 1;
}

RVA(0x001384f0, 0x3b)
void CGruntzSoundZ::Shutdown() {
    StopAndFlush();
    if (m_pCurrent != 0) {
        m_pCurrent->Stop();
    }
    StopAndFlush();
    m_digHandle = 0;
    m_pCurrent = 0;
    g_ailMidiDriver = 0;
    AIL_shutdown();
}

RVA(0x00138530, 0xa2)
void CGruntzSoundZ::StopAndFlush() {
    if (m_pCurrent != 0) {
        m_pCurrent->Stop();
    }
    POSITION pos = reinterpret_cast<POSITION>((m_map.GetCount() != 0 ? -1 : 0));
    if (pos != static_cast<POSITION>(0)) {
        do {
            CString key;
            CObject* val = 0;
            m_map.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete static_cast<CGruntzSoundInnerZ*>(val);
            }
        } while (pos != static_cast<POSITION>(0));
    }
    m_map.RemoveAll();
    m_pCurrent = 0;
}

RVA(0x001385e0, 0x85)
CGruntzSoundInnerZ* CGruntzSoundZ::CreateBank2(const char* path, const char* name) {
    if (m_enabled == 0) {
        return 0;
    }
    CGruntzSoundInnerZ* inner = new CGruntzSoundInnerZ();
    if (inner->Load(path, name) == 0) {
        if (inner != 0) {
            delete inner;
        }
        return 0;
    }
    Insert(inner);
    return inner;
}

RVA(0x00138670, 0x8a)
CGruntzSoundInnerZ* CGruntzSoundZ::CreateBank(const void* buf, u32 len, const char* name) {
    if (m_enabled == 0) {
        return 0;
    }
    CGruntzSoundInnerZ* inner = new CGruntzSoundInnerZ();
    if (inner->DecodeBuf(buf, len, name) == 0) {
        if (inner != 0) {
            delete inner;
        }
        return 0;
    }
    Insert(inner);
    return inner;
}

RVA(0x00138700, 0x2d)
void CGruntzSoundZ::Insert(CGruntzSoundInnerZ* inner) {
    if (inner == 0) {
        return;
    }
    if (m_enabled == 0) {
        return;
    }
    m_map[inner->m_name] = static_cast<CObject*>(inner);
    if (m_pCurrent == 0) {
        m_pCurrent = inner;
    }
}

RVA(0x00138730, 0x41)
CGruntzSoundInnerZ* CGruntzSoundZ::FindBank(const char* key) {
    if (m_digHandle == 0) {
        return 0;
    }
    if (key == 0) {
        return 0;
    }
    if (*key == 0) {
        return 0;
    }
    CObject* result = 0;
    return m_map.Lookup(key, result) ? static_cast<CGruntzSoundInnerZ*>(result) : 0;
}

RVA(0x00138780, 0x5b)
i32 CGruntzSoundZ::PlayCreate2(const char* path, i32 playMode, const char* name) {
    if (m_enabled == 0) {
        return 0;
    }
    CGruntzSoundInnerZ* inner = CreateBank2(path, name);
    if (inner == 0) {
        return 0;
    }
    StopCurrent();
    if (inner->Play(m_digHandle, playMode) == 0) {
        return 0;
    }
    m_pCurrent = inner;
    return 1;
}

RVA(0x001387e0, 0x60)
i32 CGruntzSoundZ::PlayCreate3(const void* buf, u32 len, i32 playMode, const char* name) {
    if (m_enabled == 0) {
        return 0;
    }
    CGruntzSoundInnerZ* inner = CreateBank(buf, len, name);
    if (inner == 0) {
        return 0;
    }
    StopCurrent();
    if (inner->Play(m_digHandle, playMode) == 0) {
        return 0;
    }
    m_pCurrent = inner;
    return 1;
}

RVA(0x00138840, 0x56)
i32 CGruntzSoundZ::PlayByName(const char* name, i32 playMode) {
    if (m_enabled == 0) {
        return 0;
    }
    CGruntzSoundInnerZ* inner = FindBank(name);
    if (inner == 0) {
        return 0;
    }
    StopCurrent();
    if (inner->Play(m_digHandle, playMode) == 0) {
        return 0;
    }
    m_pCurrent = inner;
    return 1;
}

RVA(0x001388a0, 0x18)
void CGruntzSoundZ::StopCurrent() {
    if (m_pCurrent != 0) {
        m_pCurrent->Stop();
        m_pCurrent = 0;
    }
}

RVA(0x001388c0, 0x2a)
i32 CGruntzSoundZ::Restart(i32 playMode) {
    if (m_pCurrent == 0) {
        return 0;
    }
    m_pCurrent->Stop();
    return m_pCurrent->Play(m_digHandle, playMode);
}

RVA(0x001388f0, 0xf)
i32 CGruntzSoundZ::StopAll() {
    if (m_pCurrent == 0) {
        return 0;
    }
    return m_pCurrent->StopAll();
}

RVA(0x00138900, 0x19)
i32 CGruntzSoundZ::StopBank(i32 bank) {
    if (m_pCurrent == 0) {
        return 0;
    }
    return m_pCurrent->StopBank(bank);
}

RVA(0x00138920, 0xf)
i32 CGruntzSoundZ::IsPlaying() {
    if (m_pCurrent == 0) {
        return 0;
    }
    return m_pCurrent->Stop();
}

RVA(0x00138950, 0x70)
i32 CGruntzSoundZ::SetXMidiVolume(i32 volume) {
    if (g_ailMidiDriver == 0) {
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
    AIL_set_XMIDI_master_volume(g_ailMidiDriver, scaled);
    return 1;
}

// ---------------------------------------------------------------------------
// GetXMidiVolume: read the AIL XMIDI master volume and rescale 0..127 -> 0..100.
// __thiscall (reached as m_sound->GetXMidiVolume), `this` unused.
// @early-stop
// divide-by-127 scheduling wall (~91%): logic byte-faithful (the g_ailMidiDriver
// gate, the AIL read, the 0/0x64 clamps, and the v*100/127 magic-multiply are all
// correct); the residual is MSVC's register/scheduling choice inside the reciprocal
// divide (the lea/lea/shl v*100 chain and the 0x81020409 imul). Re-homing to a
// __thiscall method is byte-neutral (`this` unused).
RVA(0x001389c0, 0x47)
i32 CGruntzSoundZ::GetXMidiVolume() {
    if (g_ailMidiDriver == 0) {
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

RVA(0x00138a50, 0x46)
CGruntzSoundInnerZ::~CGruntzSoundInnerZ() {
    ReleaseHandle();
}

RVA(0x00138c20, 0x122)
i32 CGruntzSoundInnerZ::DecodeBuf(const void* buf, u32 len, const char* name) {
    if (buf == 0) {
        return 0;
    }
    if (len < 4) {
        return 0;
    }
    if (g_ailMidiDriver == 0) {
        return 0;
    }
    ++g_midiSeqCounter;
    m_playMode = 0;
    m_tempoPct = 100;
    m_volumePct = 100;
    if (name != 0) {
        strcpy(m_name, name);
    } else {
        sprintf(m_name, "MIDI%i", g_midiSeqCounter);
    }
    if (m_loadBuffer == 0) {
        m_loadBuffer = static_cast<char*>(operator new(len));
        if (m_loadBuffer == 0) {
            return 0;
        }
        memcpy(m_loadBuffer, buf, len);
    }
    m_seqHandle = AIL_allocate_sequence_handle(g_ailMidiDriver);
    if (m_seqHandle == 0) {
        return 0;
    }
    if (AIL_init_sequence(m_seqHandle, m_loadBuffer, 0) == 0) {
        AIL_release_sequence_handle(m_seqHandle);
        m_seqHandle = 0;
        return 0;
    }
    return 1;
}

RVA(0x00138d50, 0x74)
i32 CGruntzSoundInnerZ::LoadSpecial(const char* resName, const char* name) {
    HRSRC rsrc = FindResourceA(reinterpret_cast<HMODULE>(g_ailDriver64), resName, "MIDI");
    if (rsrc == 0) {
        return 0;
    }
    HGLOBAL hRes = ::LoadResource(reinterpret_cast<HMODULE>(g_ailDriver64), rsrc);
    if (hRes == 0) {
        return 0;
    }
    void* p = LockResource(hRes);
    if (p == 0) {
        return 0;
    }
    u32 size = SizeofResource(reinterpret_cast<HMODULE>(g_ailDriver64), rsrc);
    return DecodeBuf(p, size, name);
}

RVA(0x00138dd0, 0x36)
void CGruntzSoundInnerZ::ReleaseHandle() {
    Stop();
    if (m_seqHandle != 0) {
        AIL_release_sequence_handle(m_seqHandle);
        m_seqHandle = 0;
    }
    if (m_loadBuffer != 0) {
        operator delete(m_loadBuffer);
        m_loadBuffer = 0;
    }
}

RVA(0x00138e10, 0x4a)
i32 CGruntzSoundInnerZ::Play(i32 hDriver, i32 mode) {
    if (IsStarted() == 0) {
        return 0;
    }
    m_playDriver = hDriver;
    m_playMode = mode;
    AIL_start_sequence(m_seqHandle);
    if (mode != 0) {
        AIL_set_sequence_loop_count(m_seqHandle, 0);
    }
    m_pauseDepth = 0;
    return 1;
}

RVA(0x00138e60, 0x26)
i32 CGruntzSoundInnerZ::Stop() {
    if (IsStarted() == 0) {
        return 0;
    }
    AIL_end_sequence(m_seqHandle);
    m_pauseDepth = 0;
    return 1;
}

RVA(0x00138e90, 0x3a)
i32 CGruntzSoundInnerZ::StopAll() {
    if (IsStarted() == 0) {
        return 0;
    }
    if (IsBusy() == 0) {
        return 0;
    }
    if (m_pauseDepth == 0) {
        AIL_stop_sequence(m_seqHandle);
    }
    m_pauseDepth++;
    return 1;
}

RVA(0x00138ed0, 0x4f)
i32 CGruntzSoundInnerZ::StopBank(i32 bank) {
    if (IsStarted() == 0) {
        return 0;
    }
    if (IsBusy() != 0) {
        return 1;
    }
    if (m_pauseDepth > 0) {
        m_pauseDepth--;
        if (bank != 0) {
            m_pauseDepth = 0;
        }
        if (m_pauseDepth <= 0) {
            AIL_resume_sequence(m_seqHandle);
        }
    }
    return 1;
}

RVA(0x00138f20, 0x3a)
i32 CGruntzSoundInnerZ::Retrigger() {
    if (!IsStarted()) {
        return 0;
    }
    if (IsBusy()) {
        return 0;
    }
    m_pauseDepth = 0;
    Play(m_playDriver, m_playMode);
    return 1;
}

RVA(0x00138f60, 0x2d)
i32 CGruntzSoundInnerZ::IsBusy() {
    if (IsStarted() == 0) {
        return 0;
    }
    i32 status = AIL_sequence_status(m_seqHandle);
    if (status == 4 || status == 0x10) {
        return 1;
    }
    return 0;
}

RVA(0x00138f90, 0x32)
i32 CGruntzSoundInnerZ::SetTempo(i32 tempo, i32 ms) {
    if (IsStarted() == 0) {
        return 0;
    }
    AIL_set_sequence_tempo(m_seqHandle, tempo, ms);
    m_tempoPct = tempo;
    return 1;
}

RVA(0x00138fd0, 0x5e)
i32 CGruntzSoundInnerZ::SetVolume(i32 volume, i32 ms) {
    if (IsStarted() == 0) {
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
    AIL_set_sequence_volume(m_seqHandle, scaled, ms);
    m_volumePct = volume;
    return 1;
}

RVA(0x00139030, 0x4c)
i32 CGruntzSoundInnerZ::SetLoop(i32 loop) {
    if (IsStarted() == 0) {
        return 0;
    }
    if (m_playMode != loop) {
        m_playMode = loop;
        if (loop != 0) {
            AIL_set_sequence_loop_count(m_seqHandle, 0);
        } else {
            AIL_set_sequence_loop_count(m_seqHandle, 1);
        }
    }
    return 1;
}
