#ifndef GRUNTZ_DSNDMGR_MIDIMANAGER_H
#define GRUNTZ_DSNDMGR_MIDIMANAGER_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Wap32/Object.h>

typedef struct _SEQUENCE* HSEQUENCE;

typedef struct _MDI_DRIVER* MidiDriverHandle;

extern MidiDriverHandle g_ailMidiDriver;

void __stdcall IgnoreMciNotification(WPARAM notifyCode, LPARAM deviceId);

class MidiSequence : public CObject {
public:
    virtual ~MidiSequence() OVERRIDE;

    virtual i32 LoadBuffer(const void* data, u32 dataBytes, const char* name);

    virtual i32 LoadFile(const char* path, const char* name);
    virtual void Unload();
    virtual i32 IsLoaded();
    virtual i32 Play(HWND ownerWindow, i32 looping);
    virtual i32 Pause();
    virtual i32 Resume(i32 resumeAll);
    virtual i32 End();
    virtual i32 RestartIfIdle();

    virtual i32 IsMidiSequence();
    virtual i32 LoadResource(const char* resourceName, const char* name);

    MidiSequence() {
        m_name[0] = 0;
        m_pauseDepth = 0;
        m_looping = 0;
        m_ownerWindow = NULL;
        m_tempoPct = 0x64;
        m_volumePct = 0x64;
        m_sequenceHandle = NULL;
        m_ownedData = NULL;
    }

    i32 IsPlaying();

    i32 SetTempoPercent(i32 tempoPct, i32 durationMs);
    i32 SetVolumePercent(i32 volumePct, i32 durationMs);
    i32 SetLooping(i32 looping);

    char m_name[0x40];
    i32 m_pauseDepth;
    i32 m_looping;
    HWND m_ownerWindow;
    i32 m_volumePct;
    i32 m_tempoPct;
    HSEQUENCE m_sequenceHandle;
    char* m_ownedData;
};

class MidiManager {
public:
    MidiManager() {
        m_currentSequence = NULL;
        m_ownerWindow = NULL;
    }

    ~MidiManager() {
        Shutdown();
    }

    i32 Initialize(HINSTANCE instanceHandle, HWND ownerWindow, i32 disableMidi);
    void Shutdown();
    void ClearSequences();

    i32 SetMasterVolume(i32 volumePct);
    i32 GetMasterVolume();
    MidiSequence* LoadFile(const char* path, const char* name);
    MidiSequence* LoadBuffer(const void* data, u32 dataBytes, const char* name);
    void RegisterSequence(MidiSequence* sequence);
    MidiSequence* FindSequence(const char* name);
    i32 LoadAndPlayFile(const char* path, i32 looping, const char* name);
    i32 LoadAndPlayBuffer(const void* data, u32 dataBytes, i32 looping, const char* name);
    i32 PlaySequence(const char* name, i32 looping);
    void EndAndClearCurrent();
    i32 RestartCurrent(i32 looping);
    i32 PauseCurrent();
    i32 ResumeCurrent(i32 resumeAll);
    i32 EndCurrent();
    i32 RestartCurrentIfIdle();

    CMapStringToOb m_sequences;
    MidiSequence* m_currentSequence;
    HWND m_ownerWindow;
    HINSTANCE m_instanceHandle;
    i32 m_midiAvailable;
};

#endif // GRUNTZ_DSNDMGR_MIDIMANAGER_H
