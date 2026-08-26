#ifndef DSNDMGR_SOUNDBUFFER_H
#define DSNDMGR_SOUNDBUFFER_H

#include <rva.h>

#include <Dsndmgr/IntrusiveList.h>

#include <stdio.h>

struct IDirectSound;
struct IDirectSoundBuffer;
class SoundDevice;
class SoundBuffer;
struct RiffWaveHeader;
struct WaveFormatX;

struct SoundBufferNode : public IntrusiveLink {
    SoundBuffer* m_buffer;
};

struct SoundSampleList : public IntrusiveList {
    RVA(0x001364e0, 0x1)
    ~SoundSampleList() {}
};

struct SoundBufferInstanceList : public IntrusiveList {
    RVA(0x00135ba0, 0x1)
    ~SoundBufferInstanceList() {}
};

class SoundBuffer {
public:
    SoundBuffer(IDirectSoundBuffer* buffer, SoundDevice* owner);
    virtual ~SoundBuffer();

    i32 Restore();
    i32 ReacquireBuffer();
    i32 StopAndRewind();
    i32 IsPlaying();
    i32 IsLooping();
    i32 IsInHardware();
    void SetLooping(b32 enabled);
    i32 IsLoopingEnabled();
    i32 SetVolume(i32 attenuation);
    i32 SetVolumePercent(i32 volumePct);
    i32 GetVolume();
    i32 GetVolumePercent();
    i32 GetPanPercent();
    i32 RampVolumeTo(i32 targetVolumePct, i32 durationMs, b32 stopAndRewind);
    i32 SetPan(i32 pan);
    i32 SetPanPercent(i32 panPct);
    i32 GetPan();
    i32 SetFrequency(u32 frequency);
    u32 GetFrequency();
    u32 GetBaseFrequency();
    i32 SetFrequencyOffsetPercent(i32 percentOffset);
    void UpdateDuration();
    i32 Unlock(u8* audioPtr1, u32 audioBytes1, u8* audioPtr2, u32 audioBytes2);

    i32 GetCurrentPosition(unsigned long* playCursor, unsigned long* writeCursor);
    i32 SetCurrentPosition(u32 position);
    i32 GetFormat(WaveFormatX* outFormat, u32 formatBytes, unsigned long* writtenBytes);
    i32 LoadFromFile(FILE* file, u32 bytes, i32 offset);
    i32 LockConvert(u8* sourceAudio, u32 lockBytes, b32 convert16To8);
    i32 Play();
    i32 ApplyAndPlay(i32 volumePct, i32 panPct, i32 frequencyOffsetPct, b32 looping);
    i32 Lock(
        u32 offset,
        u32 bytes,
        u8** audioPtr1,
        unsigned long* audioBytes1,
        u8** audioPtr2,
        unsigned long* audioBytes2,
        u32 flags
    );

    static void ReportError(char* file, i32 line, i32 hr);

    IntrusiveLink m_link;
    IDirectSoundBuffer* m_buffer;
    SoundDevice* m_owner;
    u32 m_playFlags;
    unsigned long m_baseFrequency;
    long m_pan;

    long m_volume;
    u32 m_frequency;
    u32 m_durationMs;
    u32 m_sampleCount;

    i32(__cdecl* m_reacquireCb)(SoundBuffer*, i32);
    i32 m_reacquireArg;
    i32 m_baseSampleRate;
    u32 m_sampleRate;
    u32 m_caps;
    SoundBufferNode m_instanceNode;
    b32 m_reusable;
    SoundBuffer* m_restoreSource;
};

class SoundBufferInstance : public SoundBuffer {
public:
    SoundBufferInstance(IDirectSoundBuffer* buffer, SoundDevice* owner);

    SoundBufferInstance(IDirectSoundBuffer* buffer, SoundDevice* owner, SoundBuffer* original);
    virtual ~SoundBufferInstance() OVERRIDE;
};

class SoundSample : public SoundBufferInstance {
public:
    SoundSample(IDirectSoundBuffer* buffer, SoundDevice* owner);
    virtual ~SoundSample() OVERRIDE;

    SoundBuffer* CreateInstance(b32 reusable);
    void DestroyInstance(SoundBuffer* instance);
    void StopAllInstances();

    SoundBuffer* AcquireInstance();
    i32 Play();
    i32 AcquireAndPlay(i32 volumePct, i32 panPct, i32 frequencyOffsetPct, b32 looping);

    SoundBufferInstanceList m_instances;
};

i32 ParseWaveChunks(RiffWaveHeader* riff, WaveFormatX** outFormat, u8** outData, u32* outDataBytes);

i32 ConvertVolumeToPercent(i32 attenuation);

extern const double c_volumePercentScale;
extern const double c_volumeCurveUnit;
extern const double c_decibelScale;
extern const double c_attenuationBase;
#endif // DSNDMGR_SOUNDBUFFER_H
