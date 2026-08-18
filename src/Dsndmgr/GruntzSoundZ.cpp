#include <rva.h>

#include <Dsndmgr/GruntzSoundZ.h>

#include <Dsndmgr/SoundBankLoad.h>
#include <Dsndmgr/VolumeScale.h>
#include <Enums.h>

#include <mss.h>
#include <stdio.h>
#include <string.h>

DATA(0x001ee8ec)
const char g_dot[] = ".";

DATA(0x00253c5c)
HMDIDRIVER g_ailMidiDriver = NULL;
DATA(0x00253c60)
i32 g_midiSeqCounter = 0;
DATA(0x00253c64)

HINSTANCE g_midiResModule = NULL;

// @identity-TODO ?1CGruntzSoundZ - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (34 fns) came from the static library. It belongs to another compiland.
RVA(0x00086040, 0x49)
CGruntzSoundZ::~CGruntzSoundZ() {
    Shutdown();
}

RVA(0x00138490, 0x5e)
i32 CGruntzSoundZ::Init(HINSTANCE hInst, HWND hwnd, i32 noMidi) {
    m_hInstance = hInst;
    m_ownerWnd = hwnd;
    m_pCurrent = NULL;
    m_enabled = 1;
    g_midiResModule = hInst;
    if (noMidi != 0) {
        m_enabled = 0;
    } else {
        AIL_startup();
        if (AIL_midiOutOpen(&g_ailMidiDriver, 0, -1) != 0 || g_ailMidiDriver == NULL) {
            m_enabled = 0;
        }
    }
    return 1;
}

RVA(0x001384f0, 0x3b)
void CGruntzSoundZ::Shutdown() {
    StopAndFlush();
    if (m_pCurrent != NULL) {
        m_pCurrent->Stop();
    }
    StopAndFlush();
    m_ownerWnd = NULL;
    m_pCurrent = NULL;
    g_ailMidiDriver = NULL;
    AIL_shutdown();
}

RVA(0x00138530, 0xa2)
void CGruntzSoundZ::StopAndFlush() {
    if (m_pCurrent != NULL) {
        m_pCurrent->Stop();
    }
    POSITION pos = m_map.GetStartPosition();
    if (pos != static_cast<POSITION>(0)) {
        do {
            CString key;
            CObject* val = 0;
            m_map.GetNextAssoc(pos, key, val);
            if (val != NULL) {
                delete static_cast<CGruntzSoundInnerZ*>(val);
            }
        } while (pos != static_cast<POSITION>(0));
    }
    m_map.RemoveAll();
    m_pCurrent = NULL;
}

RVA(0x001385e0, 0x85)
CGruntzSoundInnerZ* CGruntzSoundZ::CreateBank2(const char* path, const char* name) {
    if (m_enabled == 0) {
        return 0;
    }
    CGruntzSoundInnerZ* inner = new CGruntzSoundInnerZ();
    if (inner->Load(path, name) == 0) {
        if (inner != NULL) {
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
        if (inner != NULL) {
            delete inner;
        }
        return 0;
    }
    Insert(inner);
    return inner;
}

RVA(0x00138700, 0x2d)
void CGruntzSoundZ::Insert(CGruntzSoundInnerZ* inner) {
    if (inner == NULL) {
        return;
    }
    if (m_enabled == 0) {
        return;
    }
    m_map[inner->m_name] = static_cast<CObject*>(inner);
    if (m_pCurrent == NULL) {
        m_pCurrent = inner;
    }
}

RVA(0x00138730, 0x41)
CGruntzSoundInnerZ* CGruntzSoundZ::FindBank(const char* key) {
    if (m_ownerWnd == NULL) {
        return 0;
    }
    if (key == NULL) {
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
    if (inner == NULL) {
        return 0;
    }
    StopCurrent();
    if (inner->Play(m_ownerWnd, playMode) == 0) {
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
    if (inner == NULL) {
        return 0;
    }
    StopCurrent();
    if (inner->Play(m_ownerWnd, playMode) == 0) {
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
    if (inner == NULL) {
        return 0;
    }
    StopCurrent();
    if (inner->Play(m_ownerWnd, playMode) == 0) {
        return 0;
    }
    m_pCurrent = inner;
    return 1;
}

RVA(0x001388a0, 0x18)
void CGruntzSoundZ::StopCurrent() {
    if (m_pCurrent != NULL) {
        m_pCurrent->Stop();
        m_pCurrent = NULL;
    }
}

RVA(0x001388c0, 0x2a)
i32 CGruntzSoundZ::Restart(i32 playMode) {
    if (m_pCurrent == NULL) {
        return 0;
    }
    m_pCurrent->Stop();
    return m_pCurrent->Play(m_ownerWnd, playMode);
}

RVA(0x001388f0, 0xf)
i32 CGruntzSoundZ::StopAll() {
    if (m_pCurrent == NULL) {
        return 0;
    }
    return m_pCurrent->StopAll();
}

RVA(0x00138900, 0x19)
i32 CGruntzSoundZ::StopBank(i32 bank) {
    if (m_pCurrent == NULL) {
        return 0;
    }
    return m_pCurrent->StopBank(bank);
}

RVA(0x00138920, 0xf)
i32 CGruntzSoundZ::IsPlaying() {
    if (m_pCurrent == NULL) {
        return 0;
    }
    return m_pCurrent->Stop();
}

RVA(0x00138940, 0x3)
void __stdcall EmptyMsgHook(WPARAM, LPARAM) {}

RVA(0x00138950, 0x70)
i32 CGruntzSoundZ::SetXMidiVolume(i32 volume) {
    if (g_ailMidiDriver == NULL) {
        return 0;
    }
    i32 scaled;
    if (volume <= 0) {
        scaled = 0;
    } else if (volume >= VOLUME_PCT_MAX) {
        scaled = MIDI_VOLUME_MAX;
    } else {
        scaled = volume * MIDI_VOLUME_MAX / VOLUME_PCT_MAX;
    }
    AIL_set_XMIDI_master_volume(g_ailMidiDriver, scaled);
    return 1;
}

// @early-stop
RVA(0x001389c0, 0x47)
i32 CGruntzSoundZ::GetXMidiVolume() {
    if (g_ailMidiDriver == NULL) {
        return VOLUME_PCT_MAX;
    }
    i32 v = AIL_XMIDI_master_volume(g_ailMidiDriver);
    if (v <= 0) {
        return 0;
    }
    if (v >= MIDI_VOLUME_MAX) {
        return VOLUME_PCT_MAX;
    }
    return v * VOLUME_PCT_MAX / MIDI_VOLUME_MAX;
}

RVA(0x00138a10, 0xb)
i32 CGruntzSoundInnerZ::IsStarted() {
    return m_seqHandle != NULL;
}

RVA(0x00138a20, 0x6)
i32 CGruntzSoundInnerZ::IsMidi() {
    return 1;
}

RVA_COMPGEN(0x00138a30, 0x1e, ??_GCGruntzSoundInnerZ@@UAEPAXI@Z)
RVA(0x00138a50, 0x46)
CGruntzSoundInnerZ::~CGruntzSoundInnerZ() {
    ReleaseHandle();
}

RVA(0x00138aa0, 0x175)
i32 CGruntzSoundInnerZ::Load(const char* path, const char* name) {
    if (strstr(path, g_dot) == NULL) {
        return LoadSpecial(path, name);
    }
    CFile file;
    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    u32 length = file.GetLength();
    if (length < 4) {
        return 0;
    }
    m_loadBuffer = new char[length];
    if (m_loadBuffer == NULL) {
        return 0;
    }
    if (file.Read(m_loadBuffer, length) != length) {
        return 0;
    }
    return DecodeBuf(m_loadBuffer, length, name);
}

RVA(0x00138c20, 0x122)
i32 CGruntzSoundInnerZ::DecodeBuf(const void* buf, u32 len, const char* name) {
    if (buf == NULL) {
        return 0;
    }
    if (len < 4) {
        return 0;
    }
    if (g_ailMidiDriver == NULL) {
        return 0;
    }
    ++g_midiSeqCounter;
    m_playMode = 0;
    m_tempoPct = 100;
    m_volumePct = VOLUME_PCT_MAX;
    if (name != NULL) {
        strcpy(m_name, name);
    } else {
        sprintf(m_name, "MIDI%i", g_midiSeqCounter);
    }
    if (m_loadBuffer == NULL) {
        m_loadBuffer = new char[len];
        if (m_loadBuffer == NULL) {
            return 0;
        }
        memcpy(m_loadBuffer, buf, len);
    }
    m_seqHandle = AIL_allocate_sequence_handle(g_ailMidiDriver);
    if (m_seqHandle == NULL) {
        return 0;
    }
    if (AIL_init_sequence(m_seqHandle, m_loadBuffer, 0) == 0) {
        AIL_release_sequence_handle(m_seqHandle);
        m_seqHandle = NULL;
        return 0;
    }
    return 1;
}

RVA(0x00138d50, 0x74)
i32 CGruntzSoundInnerZ::LoadSpecial(const char* resName, const char* name) {
    HRSRC rsrc = FindResourceA(g_midiResModule, resName, "MIDI");
    if (rsrc == NULL) {
        return 0;
    }
    HGLOBAL hRes = LoadResource(g_midiResModule, rsrc);
    if (hRes == NULL) {
        return 0;
    }
    const u8* p = static_cast<const u8*>(LockResource(hRes));
    if (p == NULL) {
        return 0;
    }
    u32 size = SizeofResource(g_midiResModule, rsrc);
    return DecodeBuf(p, size, name);
}

RVA(0x00138dd0, 0x36)
void CGruntzSoundInnerZ::ReleaseHandle() {
    Stop();
    if (m_seqHandle != NULL) {
        AIL_release_sequence_handle(m_seqHandle);
        m_seqHandle = NULL;
    }
    if (m_loadBuffer != NULL) {
        delete[] m_loadBuffer;
        m_loadBuffer = NULL;
    }
}

RVA(0x00138e10, 0x4a)
i32 CGruntzSoundInnerZ::Play(HWND hOwner, i32 mode) {
    if (IsStarted() == 0) {
        return 0;
    }
    m_playOwner = hOwner;
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
    Play(m_playOwner, m_playMode);
    return 1;
}

RVA(0x00138f60, 0x2d)
i32 CGruntzSoundInnerZ::IsBusy() {
    if (IsStarted() == 0) {
        return 0;
    }
    i32 status = AIL_sequence_status(m_seqHandle);
    if (status == SEQ_PLAYING || status == SEQ_PLAYINGBUTRELEASED) {
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
    } else if (volume >= VOLUME_PCT_MAX) {
        scaled = MIDI_VOLUME_MAX;
    } else {
        scaled = volume * MIDI_VOLUME_MAX / VOLUME_PCT_MAX;
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
