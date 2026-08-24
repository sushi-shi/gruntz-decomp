#include <rva.h>

#include <Dsndmgr/MidiManager.h>

#include <Dsndmgr/SoundBankLoad.h>
#include <Dsndmgr/VolumeScale.h>
#include <Enums.h>

#include <mss.h>
#include <stdio.h>
#include <string.h>

DATA(0x001ee8ec)
const char g_singleDot[] = ".";

DATA(0x00253c5c)
HMDIDRIVER g_ailMidiDriver = NULL;
DATA(0x00253c60)
i32 g_midiSequenceCounter = 0;
DATA(0x00253c64)

HINSTANCE g_midiResourceModule = NULL;

RVA(0x00138490, 0x5e)
i32 MidiManager::Initialize(HINSTANCE instanceHandle, HWND ownerWindow, i32 disableMidi) {
    m_instanceHandle = instanceHandle;
    m_ownerWindow = ownerWindow;
    m_currentSequence = NULL;
    m_midiAvailable = 1;
    g_midiResourceModule = instanceHandle;
    if (disableMidi != 0) {
        m_midiAvailable = 0;
    } else {
        AIL_startup();
        if (AIL_midiOutOpen(&g_ailMidiDriver, NULL, -1) != 0 || g_ailMidiDriver == NULL) {
            m_midiAvailable = 0;
        }
    }
    return 1;
}

RVA(0x001384f0, 0x3b)
void MidiManager::Shutdown() {
    ClearSequences();
    if (m_currentSequence != NULL) {
        m_currentSequence->End();
    }
    ClearSequences();
    m_ownerWindow = NULL;
    m_currentSequence = NULL;
    g_ailMidiDriver = NULL;
    AIL_shutdown();
}

RVA(0x00138530, 0xa2)
void MidiManager::ClearSequences() {
    if (m_currentSequence != NULL) {
        m_currentSequence->End();
    }
    POSITION pos = m_sequences.GetStartPosition();
    if (pos != static_cast<POSITION>(0)) {
        do {
            CString key;
            CObject* sequenceObject = NULL;
            m_sequences.GetNextAssoc(pos, key, sequenceObject);
            if (sequenceObject != NULL) {
                delete static_cast<MidiSequence*>(sequenceObject);
            }
        } while (pos != static_cast<POSITION>(0));
    }
    m_sequences.RemoveAll();
    m_currentSequence = NULL;
}

RVA(0x001385e0, 0x85)
MidiSequence* MidiManager::LoadFile(const char* path, const char* name) {
    if (m_midiAvailable == 0) {
        return NULL;
    }
    MidiSequence* sequence = new MidiSequence();
    if (sequence->LoadFile(path, name) == 0) {
        if (sequence != NULL) {
            delete sequence;
        }
        return NULL;
    }
    RegisterSequence(sequence);
    return sequence;
}

RVA(0x00138670, 0x8a)
MidiSequence* MidiManager::LoadBuffer(const void* data, u32 dataBytes, const char* name) {
    if (m_midiAvailable == 0) {
        return NULL;
    }
    MidiSequence* sequence = new MidiSequence();
    if (sequence->LoadBuffer(data, dataBytes, name) == 0) {
        if (sequence != NULL) {
            delete sequence;
        }
        return NULL;
    }
    RegisterSequence(sequence);
    return sequence;
}

RVA(0x00138700, 0x2d)
void MidiManager::RegisterSequence(MidiSequence* sequence) {
    if (sequence == NULL) {
        return;
    }
    if (m_midiAvailable == 0) {
        return;
    }
    m_sequences[sequence->m_name] = static_cast<CObject*>(sequence);
    if (m_currentSequence == NULL) {
        m_currentSequence = sequence;
    }
}

RVA(0x00138730, 0x41)
MidiSequence* MidiManager::FindSequence(const char* name) {
    if (m_ownerWindow == NULL) {
        return NULL;
    }
    if (name == NULL) {
        return NULL;
    }
    if (*name == 0) {
        return NULL;
    }
    CObject* sequenceObject = NULL;
    return m_sequences.Lookup(name, sequenceObject) ? static_cast<MidiSequence*>(sequenceObject)
                                                    : NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00138780, 0x5b)
i32 MidiManager::LoadAndPlayFile(const char* path, i32 looping, const char* name) {
    if (m_midiAvailable == 0) {
        return 0;
    }
    MidiSequence* sequence = LoadFile(path, name);
    if (sequence == NULL) {
        return 0;
    }
    EndAndClearCurrent();
    if (sequence->Play(m_ownerWindow, looping) == 0) {
        return 0;
    }
    m_currentSequence = sequence;
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001387e0, 0x60)
i32 MidiManager::LoadAndPlayBuffer(const void* data, u32 dataBytes, i32 looping, const char* name) {
    if (m_midiAvailable == 0) {
        return 0;
    }
    MidiSequence* sequence = LoadBuffer(data, dataBytes, name);
    if (sequence == NULL) {
        return 0;
    }
    EndAndClearCurrent();
    if (sequence->Play(m_ownerWindow, looping) == 0) {
        return 0;
    }
    m_currentSequence = sequence;
    return 1;
}

RVA(0x00138840, 0x56)
i32 MidiManager::PlaySequence(const char* name, i32 looping) {
    if (m_midiAvailable == 0) {
        return 0;
    }
    MidiSequence* sequence = FindSequence(name);
    if (sequence == NULL) {
        return 0;
    }
    EndAndClearCurrent();
    if (sequence->Play(m_ownerWindow, looping) == 0) {
        return 0;
    }
    m_currentSequence = sequence;
    return 1;
}

RVA(0x001388a0, 0x18)
void MidiManager::EndAndClearCurrent() {
    if (m_currentSequence != NULL) {
        m_currentSequence->End();
        m_currentSequence = NULL;
    }
}

RVA(0x001388c0, 0x2a)
i32 MidiManager::RestartCurrent(i32 looping) {
    if (m_currentSequence == NULL) {
        return 0;
    }
    m_currentSequence->End();
    return m_currentSequence->Play(m_ownerWindow, looping);
}

RVA(0x001388f0, 0xf)
i32 MidiManager::PauseCurrent() {
    if (m_currentSequence == NULL) {
        return 0;
    }
    return m_currentSequence->Pause();
}

RVA(0x00138900, 0x19)
i32 MidiManager::ResumeCurrent(i32 resumeAll) {
    if (m_currentSequence == NULL) {
        return 0;
    }
    return m_currentSequence->Resume(resumeAll);
}

RVA(0x00138920, 0xf)
i32 MidiManager::EndCurrent() {
    if (m_currentSequence == NULL) {
        return 0;
    }
    return m_currentSequence->End();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00138930, 0xf)
i32 MidiManager::RestartCurrentIfIdle() {
    if (m_currentSequence == NULL) {
        return 0;
    }
    return m_currentSequence->RestartIfIdle();
}

RVA(0x00138940, 0x3)
void __stdcall IgnoreMciNotification(WPARAM notifyCode, LPARAM deviceId) {}

RVA(0x00138950, 0x70)
i32 MidiManager::SetMasterVolume(i32 volumePct) {
    if (g_ailMidiDriver == NULL) {
        return 0;
    }
    i32 scaled;
    if (volumePct <= 0) {
        scaled = 0;
    } else if (volumePct >= VOLUME_PCT_MAX) {
        scaled = MIDI_VOLUME_MAX;
    } else {
        scaled = volumePct * MIDI_VOLUME_MAX / VOLUME_PCT_MAX;
    }
    AIL_set_XMIDI_master_volume(g_ailMidiDriver, scaled);
    return 1;
}

RVA(0x001389c0, 0x47)
i32 MidiManager::GetMasterVolume() {
    if (g_ailMidiDriver == NULL) {
        return VOLUME_PCT_MAX;
    }
    return MidiVolumeToPercent(AIL_XMIDI_master_volume(g_ailMidiDriver));
}

RVA(0x00138a10, 0xb)
i32 MidiSequence::IsLoaded() {
    return m_sequenceHandle != NULL;
}

RVA(0x00138a20, 0x6)
i32 MidiSequence::IsMidiSequence() {
    return 1;
}

RVA_COMPGEN(0x00138a30, 0x1e, ??_GMidiSequence@@UAEPAXI@Z)
RVA(0x00138a50, 0x46)
MidiSequence::~MidiSequence() {
    Unload();
}

RVA(0x00138aa0, 0x175)
i32 MidiSequence::LoadFile(const char* path, const char* name) {
    if (strstr(path, g_singleDot) == NULL) {
        return LoadResource(path, name);
    }
    CFile file;
    if (!file.Open(path, 0, NULL)) {
        return 0;
    }
    u32 length = file.GetLength();
    if (length < 4) {
        return 0;
    }
    m_ownedData = new char[length];
    if (m_ownedData == NULL) {
        return 0;
    }
    if (file.Read(m_ownedData, length) != length) {
        return 0;
    }
    return LoadBuffer(m_ownedData, length, name);
}

RVA(0x00138c20, 0x122)
i32 MidiSequence::LoadBuffer(const void* data, u32 dataBytes, const char* name) {
    if (data == NULL) {
        return 0;
    }
    if (dataBytes < 4) {
        return 0;
    }
    if (g_ailMidiDriver == NULL) {
        return 0;
    }
    ++g_midiSequenceCounter;
    m_looping = 0;
    m_tempoPct = 100;
    m_volumePct = VOLUME_PCT_MAX;
    if (name != NULL) {
        strcpy(m_name, name);
    } else {
        sprintf(m_name, "MIDI%i", g_midiSequenceCounter);
    }
    if (m_ownedData == NULL) {
        m_ownedData = new char[dataBytes];
        if (m_ownedData == NULL) {
            return 0;
        }
        memcpy(m_ownedData, data, dataBytes);
    }
    m_sequenceHandle = AIL_allocate_sequence_handle(g_ailMidiDriver);
    if (m_sequenceHandle == NULL) {
        return 0;
    }
    if (AIL_init_sequence(m_sequenceHandle, m_ownedData, 0) == 0) {
        AIL_release_sequence_handle(m_sequenceHandle);
        m_sequenceHandle = NULL;
        return 0;
    }
    return 1;
}

RVA(0x00138d50, 0x74)
i32 MidiSequence::LoadResource(const char* resourceName, const char* name) {
    HRSRC resourceInfo = FindResourceA(g_midiResourceModule, resourceName, "MIDI");
    if (resourceInfo == NULL) {
        return 0;
    }
    HGLOBAL resourceData = ::LoadResource(g_midiResourceModule, resourceInfo);
    if (resourceData == NULL) {
        return 0;
    }
    const u8* data = static_cast<const u8*>(LockResource(resourceData));
    if (data == NULL) {
        return 0;
    }
    u32 dataBytes = SizeofResource(g_midiResourceModule, resourceInfo);
    return LoadBuffer(data, dataBytes, name);
}

RVA(0x00138dd0, 0x36)
void MidiSequence::Unload() {
    End();
    if (m_sequenceHandle != NULL) {
        AIL_release_sequence_handle(m_sequenceHandle);
        m_sequenceHandle = NULL;
    }
    if (m_ownedData != NULL) {
        delete[] m_ownedData;
        m_ownedData = NULL;
    }
}

RVA(0x00138e10, 0x4a)
i32 MidiSequence::Play(HWND ownerWindow, i32 looping) {
    if (IsLoaded() == 0) {
        return 0;
    }
    m_ownerWindow = ownerWindow;
    m_looping = looping;
    AIL_start_sequence(m_sequenceHandle);
    if (looping != 0) {
        AIL_set_sequence_loop_count(m_sequenceHandle, 0);
    }
    m_pauseDepth = 0;
    return 1;
}

RVA(0x00138e60, 0x26)
i32 MidiSequence::End() {
    if (IsLoaded() == 0) {
        return 0;
    }
    AIL_end_sequence(m_sequenceHandle);
    m_pauseDepth = 0;
    return 1;
}

RVA(0x00138e90, 0x3a)
i32 MidiSequence::Pause() {
    if (IsLoaded() == 0) {
        return 0;
    }
    if (IsPlaying() == 0) {
        return 0;
    }
    if (m_pauseDepth == 0) {
        AIL_stop_sequence(m_sequenceHandle);
    }
    m_pauseDepth++;
    return 1;
}

RVA(0x00138ed0, 0x4f)
i32 MidiSequence::Resume(i32 resumeAll) {
    if (IsLoaded() == 0) {
        return 0;
    }
    if (IsPlaying() != 0) {
        return 1;
    }
    if (m_pauseDepth > 0) {
        m_pauseDepth--;
        if (resumeAll != 0) {
            m_pauseDepth = 0;
        }
        if (m_pauseDepth <= 0) {
            AIL_resume_sequence(m_sequenceHandle);
        }
    }
    return 1;
}

RVA(0x00138f20, 0x3a)
i32 MidiSequence::RestartIfIdle() {
    if (!IsLoaded()) {
        return 0;
    }
    if (IsPlaying()) {
        return 0;
    }
    m_pauseDepth = 0;
    Play(m_ownerWindow, m_looping);
    return 1;
}

RVA(0x00138f60, 0x2d)
i32 MidiSequence::IsPlaying() {
    if (IsLoaded() == 0) {
        return 0;
    }
    i32 status = AIL_sequence_status(m_sequenceHandle);
    if (status == SEQ_PLAYING || status == SEQ_PLAYINGBUTRELEASED) {
        return 1;
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00138f90, 0x32)
i32 MidiSequence::SetTempoPercent(i32 tempoPct, i32 durationMs) {
    if (IsLoaded() == 0) {
        return 0;
    }
    AIL_set_sequence_tempo(m_sequenceHandle, tempoPct, durationMs);
    m_tempoPct = tempoPct;
    return 1;
}

RVA(0x00138fd0, 0x5e)
i32 MidiSequence::SetVolumePercent(i32 volumePct, i32 durationMs) {
    if (IsLoaded() == 0) {
        return 0;
    }
    i32 scaled;
    if (volumePct <= 0) {
        scaled = 0;
    } else if (volumePct >= VOLUME_PCT_MAX) {
        scaled = MIDI_VOLUME_MAX;
    } else {
        scaled = volumePct * MIDI_VOLUME_MAX / VOLUME_PCT_MAX;
    }
    AIL_set_sequence_volume(m_sequenceHandle, scaled, durationMs);
    m_volumePct = volumePct;
    return 1;
}

RVA(0x00139030, 0x4c)
i32 MidiSequence::SetLooping(i32 looping) {
    if (IsLoaded() == 0) {
        return 0;
    }
    if (m_looping != looping) {
        m_looping = looping;
        if (looping != 0) {
            AIL_set_sequence_loop_count(m_sequenceHandle, 0);
        } else {
            AIL_set_sequence_loop_count(m_sequenceHandle, 1);
        }
    }
    return 1;
}
