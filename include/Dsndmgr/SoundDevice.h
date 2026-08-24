#ifndef DSNDMGR_SOUNDDEVICE_H
#define DSNDMGR_SOUNDDEVICE_H

#include <rva.h>

#include <Mfc.h>

#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/SoundTask.h>
#include <Dsndmgr/WaveFormatX.h>
#include <Enums.h>

class SoundDevice;

class SoundSample;

struct StreamVoice;

class SoundDevice {
public:
    SoundDevice();
    virtual ~SoundDevice();

    void Shutdown();
    void DestroyBuffer(SoundBuffer* buffer);
    void StopAllBuffers();
    i32 ClearVolumeRamps();
    i32 SetPrimaryFormat(WaveFormatX* format);

    i32 StartPrimaryBuffer();
    i32 CreatePrimaryBuffer();
    IDirectSoundBuffer* GetPrimary();
    SoundSample* CreateSample(WaveFormatX* format, u32 bytes, u32 flags);
    SoundSample* LoadSampleFile(char* path, u32 flags, u32 loadOptions);
    SoundSample* LoadSample(RiffWaveHeader* riff, u32 flags, u32 loadOptions);
    SoundSample* LoadSampleResource(const char* name, u32 flags, u32 loadOptions);
    i32 ReloadResource(SoundBuffer* buffer, const char* name, u32 loadOptions);
    i32 ValidateRestore(SoundBuffer* buffer, WaveFormatX* format, u32 formatBytes);
    i32 ReloadRiff(SoundBuffer* buffer, RiffWaveHeader* riff, u32 loadOptions);
    i32 ReloadFile(SoundBuffer* buffer, char* path, u32 loadOptions);

    i32 Initialize(HWND hwnd, u32 cooperativeLevel, u32 bufferFlags);
    i32 Compact();
    i32 ReacquireViaCallback();
    i32 SetCooperativeLevel(HWND hwnd, u32 cooperativeLevel);

    i32 TickVolumeRamps(i32 timestampMs);

    static i32 VolumeToAttenuation(i32 volumePct);
    static void BuildVolumeTable();

    SoundSampleList m_samples;
    SoundTaskList m_volumeRamps;
    IDirectSound* m_device;

    char m_reserved[0x78 - 0x18];
    i32 m_initialized;
    i32 m_lastRampTickMs;

    i32 (SoundDevice::*m_reacquireProc)();
    IDirectSoundBuffer* m_primaryBuffer;
    i32 m_cooperativeLevel;
    u32 m_bufferFlags;
    i32 m_force8Bit;
};

#endif // DSNDMGR_SOUNDDEVICE_H
