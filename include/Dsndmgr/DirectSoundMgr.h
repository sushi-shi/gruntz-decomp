#ifndef DSNDMGR_DIRECTSOUNDMGR_H
#define DSNDMGR_DIRECTSOUNDMGR_H

#include <rva.h>

#include <Dsndmgr/SoundVoiceList.h>

#include <stdio.h>

struct IDirectSound;
struct IDirectSoundBuffer;
class SoundDevice;
class DirectSoundMgr;
struct RiffWaveHeader;
struct WaveFormatX;

struct CloneNode : public DSoundLink {
    DirectSoundMgr* m_inst;
};

class DirectSoundMgr {
public:
    DirectSoundMgr(IDirectSoundBuffer* buf, SoundDevice* owner);
    virtual ~DirectSoundMgr();

    i32 Restore();
    i32 ReacquireBuffer();
    i32 StopAndRewind();
    i32 IsPlaying();
    i32 IsLooping();
    i32 IsInHardware();
    void SetLooping(i32 enabled);
    i32 IsLoopingEnabled();
    i32 SetVolume(i32 vol);
    i32 SetVolumeByIndex(i32 idx);
    i32 GetVolume();
    i32 GetVolumePercent();
    i32 GetPanPercent();
    i32 CloneAndPlay(i32 key, i32 mode, i32 slot);
    i32 SetPan(i32 pan);
    i32 SetPanByIndex(i32 idx);
    i32 GetPan();
    i32 SetFrequency(u32 freq);
    u32 GetFrequency();
    u32 GetBaseFrequency();
    i32 SetFrequencyOffsetPercent(i32 percentOffset);
    void ComputeDuration();
    i32 Unlock(u8* audioPtr1, u32 audioBytes1, u8* audioPtr2, u32 audioBytes2);

    i32 GetCurrentPosition(unsigned long* play, unsigned long* write);
    i32 SetCurrentPosition(u32 pos);
    i32 GetFormat(WaveFormatX* fmt, u32 size, unsigned long* written);
    i32 LoadFromFile(FILE* fp, u32 bytes, i32 offset);
    i32 LockConvert(u8* src, u32 lockBytes, u32 convert);
    i32 Play();
    i32 ApplyAndPlay(i32 vol, i32 pan, i32 freqPct, i32 loop);
    i32 Lock(
        u32 off,
        u32 bytes,
        u8** audioPtr1,
        unsigned long* audioBytes1,
        u8** audioPtr2,
        unsigned long* audioBytes2,
        u32 flags
    );

    static void GetErrorString(char* file, i32 line, i32 hr);

    DSoundLink m_link;
    IDirectSoundBuffer* m_buffer;
    SoundDevice* m_owner;
    u32 m_playFlags;
    unsigned long m_freq;
    long m_pan;

    long m_volume;
    u32 m_setFreq;
    u32 m_durationMs;
    u32 m_sampleCount;

    i32(__cdecl* m_reacquireCb)(DirectSoundMgr*, i32);
    i32 m_reacquireCtx;
    i32 m_rateBase;
    u32 m_sampleRate;
    u32 m_caps;
    CloneNode m_cloneNode;
    i32 m_playKey;
    DirectSoundMgr* m_reacquireOwner;
};

class DSoundBaseSub : public DirectSoundMgr {
public:
    DSoundBaseSub(IDirectSoundBuffer* buf, SoundDevice* owner);

    DSoundBaseSub(IDirectSoundBuffer* buf, SoundDevice* owner, DirectSoundMgr* original);
    virtual ~DSoundBaseSub() OVERRIDE;
};

class DSoundCloneInst : public DSoundBaseSub {
public:
    DSoundCloneInst(IDirectSoundBuffer* buf, SoundDevice* owner);
    virtual ~DSoundCloneInst() OVERRIDE;

    DirectSoundMgr* Clone(i32 a);
    void RemoveClone(DirectSoundMgr* clone);
    void StopAllClones();

    DirectSoundMgr* GetItem();
    i32 Play();
    i32 ConfigureItem(i32 vol, i32 pan, i32 freqPct, i32 loop);

    DSoundCloneList m_cloneList;
};

i32 ParseWaveChunks(RiffWaveHeader* riff, WaveFormatX** fmtOut, u8** dataOut, u32* sizeOut);

i32 ConvertVolumeToPercent(i32 v);

extern const double c_volScale;
extern const double c_volNum;
extern const double c_powExp;
extern const double c_acosNorm;
#endif // DSNDMGR_DIRECTSOUNDMGR_H
