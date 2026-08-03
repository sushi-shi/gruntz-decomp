#ifndef DSNDMGR_SOUNDDEVICE_H
#define DSNDMGR_SOUNDDEVICE_H

#include <rva.h>

#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/SoundVoiceList.h>
#include <Dsndmgr/WaveFormatX.h>
#include <Enums.h>

class SoundDevice;

class DSoundCloneInst;

struct StreamVoice;

class SoundDevice {
public:
    SoundDevice();
    virtual ~SoundDevice();

    void Shutdown();
    void RemoveBuffer(DirectSoundMgr* node);
    void StopAll();
    i32 FreeSamples();
    i32 SetPrimaryFormat(void* fmt);

    i32 StartPrimary();
    i32 CreatePrimaryBuffer();
    IDirectSoundBuffer* GetPrimary();
    DSoundCloneInst* CreateBuffer(WaveFormatX* fmt, u32 bytes, u32 flags);
    DSoundCloneInst* AcquireFile(char* path, u32 flags, u32 loadOpts);
    DSoundCloneInst* Acquire(void* riff, u32 flags, u32 loadOpts);
    DSoundCloneInst* AcquireResource(const char* name, u32 flags, u32 loadOpts);
    i32 ReloadResource(DirectSoundMgr* probe, const char* name, u32 loadOpts);
    i32 ValidateRestore(DirectSoundMgr* buf, WaveFormatX* fmt, u32 size);
    i32 ReloadRiff(DirectSoundMgr* buf, void* riff, u32 loadOpts);
    i32 ReloadFile(DirectSoundMgr* buf, char* path, u32 loadOpts);

    i32 Create(void* hwnd, u32 level, u32 flags);
    i32 Compact();
    i32 ReacquireViaCallback();
    i32 SetCooperativeLevel(void* hwnd, u32 level);

    i32 PurgeVoiceList(i32 time);

    static i32 VolumeToAttenuation(i32 value);
    static void BuildVolumeTable();

    DSoundList m_bufferList;
    DSoundList m_voiceList;
    IDirectSound* m_device;

    char m_reserved[0x78 - 0x18];
    i32 m_initialized;
    i32 m_createFlag;

    i32 (SoundDevice::*m_reacquireProc)();
    IDirectSoundBuffer* m_primaryBuffer;
    i32 m_coopLevel;
    u32 m_bufferFlags;
    i32 m_force8Bit;
};
SIZE(0x94);

#endif // DSNDMGR_SOUNDDEVICE_H
