#include <StdAfx.h>

#include <Ints.h>

#include <Dsndmgr/MidiManager.h>

#include <Dsndmgr/SoundBankLoad.h>
#include <Dsndmgr/VolumeScale.h>
#include <Enums.h>
#include <SafeDelete.h>

#include <mss.h>
#include <stdio.h>
#include <string.h>

const char g_singleDot[] = ".";

HMDIDRIVER g_ailMidiDriver = NULL;

i32 g_midiSequenceCounter = 0;

HINSTANCE g_midiResourceModule = NULL;

i32 MidiManager::Initialize(HINSTANCE instanceHandle, HWND ownerWindow, b32 disableMidi) {
    m_instanceHandle = instanceHandle;
    m_ownerWindow = ownerWindow;
    m_currentSequence = NULL;
    SetEnabled(true);
    g_midiResourceModule = instanceHandle;
    if (disableMidi != false) {
        SetEnabled(false);
    } else {
        AIL_startup();
        if (AIL_midiOutOpen(&g_ailMidiDriver, NULL, static_cast<i32>(MIDI_MAPPER)) != 0
            || g_ailMidiDriver == NULL) {
            SetEnabled(false);
        }
    }
    return 1;
}

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

MidiSequence* MidiManager::LoadFile(const char* path, const char* name) {
    if (m_midiAvailable == false) {
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

MidiSequence* MidiManager::LoadBuffer(const void* data, u32 dataBytes, const char* name) {
    if (m_midiAvailable == false) {
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

void MidiManager::RegisterSequence(MidiSequence* sequence) {
    if (sequence == NULL) {
        return;
    }
    if (m_midiAvailable == false) {
        return;
    }
    m_sequences[sequence->m_name] = static_cast<CObject*>(sequence);
    if (m_currentSequence == NULL) {
        m_currentSequence = sequence;
    }
}

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

i32 MidiManager::LoadAndPlayFile(const char* path, b32 looping, const char* name) {
    if (m_midiAvailable == false) {
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

i32 MidiManager::LoadAndPlayBuffer(const void* data, u32 dataBytes, b32 looping, const char* name) {
    if (m_midiAvailable == false) {
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

i32 MidiManager::PlaySequence(const char* name, b32 looping) {
    if (m_midiAvailable == false) {
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

void MidiManager::EndAndClearCurrent() {
    if (m_currentSequence != NULL) {
        m_currentSequence->End();
        m_currentSequence = NULL;
    }
}

i32 MidiManager::RestartCurrent(b32 looping) {
    if (m_currentSequence == NULL) {
        return 0;
    }
    m_currentSequence->End();
    return m_currentSequence->Play(m_ownerWindow, looping);
}

i32 MidiManager::PauseCurrent() {
    if (m_currentSequence == NULL) {
        return 0;
    }
    return m_currentSequence->Pause();
}

i32 MidiManager::ResumeCurrent(i32 resumeAll) {
    if (m_currentSequence == NULL) {
        return 0;
    }
    return m_currentSequence->Resume(resumeAll);
}

i32 MidiManager::EndCurrent() {
    if (m_currentSequence == NULL) {
        return 0;
    }
    return m_currentSequence->End();
}

i32 MidiManager::RestartCurrentIfIdle() {
    if (m_currentSequence == NULL) {
        return 0;
    }
    return m_currentSequence->RestartIfIdle();
}

void __stdcall IgnoreMciNotification(WPARAM notifyCode, LPARAM deviceId) {}

i32 MidiManager::SetMasterVolume(i32 volumePct) {
    if (g_ailMidiDriver == NULL) {
        return 0;
    }
    AIL_set_XMIDI_master_volume(g_ailMidiDriver, PercentToMidiVolume(volumePct));
    return 1;
}

i32 MidiManager::GetMasterVolume() {
    if (g_ailMidiDriver == NULL) {
        return VOLUME_PCT_MAX;
    }
    return MidiVolumeToPercent(AIL_XMIDI_master_volume(g_ailMidiDriver));
}

i32 MidiSequence::IsLoaded() {
    return m_sequenceHandle != NULL;
}

i32 MidiSequence::IsMidiSequence() {
    return 1;
}

MidiSequence::~MidiSequence() {
    Unload();
}

i32 MidiSequence::LoadFile(const char* path, const char* name) {
    if (strstr(path, g_singleDot) == NULL) {
        return LoadResource(path, name);
    }
    CFile file;
    if (!file.Open(path, CFile::modeRead, NULL)) {
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
    m_looping = false;
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

void MidiSequence::Unload() {
    End();
    if (m_sequenceHandle != NULL) {
        AIL_release_sequence_handle(m_sequenceHandle);
        m_sequenceHandle = NULL;
    }
    SAFE_DELETE_ARRAY(m_ownedData);
}

i32 MidiSequence::Play(HWND ownerWindow, b32 looping) {
    if (IsLoaded() == 0) {
        return 0;
    }
    m_ownerWindow = ownerWindow;
    m_looping = looping;
    AIL_start_sequence(m_sequenceHandle);
    if (looping != false) {
        AIL_set_sequence_loop_count(m_sequenceHandle, 0);
    }
    m_pauseDepth = 0;
    return 1;
}

i32 MidiSequence::End() {
    if (IsLoaded() == 0) {
        return 0;
    }
    AIL_end_sequence(m_sequenceHandle);
    m_pauseDepth = 0;
    return 1;
}

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

i32 MidiSequence::SetTempoPercent(i32 tempoPct, i32 durationMs) {
    if (IsLoaded() == 0) {
        return 0;
    }
    AIL_set_sequence_tempo(m_sequenceHandle, tempoPct, durationMs);
    m_tempoPct = tempoPct;
    return 1;
}

i32 MidiSequence::SetVolumePercent(i32 volumePct, i32 durationMs) {
    if (IsLoaded() == 0) {
        return 0;
    }
    AIL_set_sequence_volume(m_sequenceHandle, PercentToMidiVolume(volumePct), durationMs);
    m_volumePct = volumePct;
    return 1;
}

i32 MidiSequence::SetLooping(b32 looping) {
    if (IsLoaded() == 0) {
        return 0;
    }
    if (m_looping != looping) {
        m_looping = looping;
        if (looping != false) {
            AIL_set_sequence_loop_count(m_sequenceHandle, 0);
        } else {
            AIL_set_sequence_loop_count(m_sequenceHandle, 1);
        }
    }
    return 1;
}
