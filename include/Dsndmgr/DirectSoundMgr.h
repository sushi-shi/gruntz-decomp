#ifndef DSNDMGR_DIRECTSOUNDMGR_H
#define DSNDMGR_DIRECTSOUNDMGR_H

#include <rva.h>
#include <stdio.h>                  // FILE (LoadFromFile stream arg)
#include <Dsndmgr/SoundVoiceList.h> // DSoundLink / DSoundList intrusive list primitive

struct IDirectSound;       // forward-decl: real dsound.h interface (dispatched in the .cpp)
struct IDirectSoundBuffer; // forward-decl: CreateSoundBuffer's out-param type
class SoundDevice;         // owning device (m_owner); full def in SoundDevice.h
class DirectSoundMgr;      // a clone (CloneNode::m_inst back-points at it)

struct CloneNode : public DSoundLink {
    DirectSoundMgr* m_inst; // +0x08  back-pointer to the owning buffer
};
SIZE(0xc); // {link.next, link.prev, inst}

class DirectSoundMgr {
public:
    DirectSoundMgr(IDirectSoundBuffer* buf, SoundDevice* owner); // 0x1351d0 ctor
    virtual ~DirectSoundMgr(); // 0x135300  base-subobject dtor (implicit vptr reset)

    i32 Restore();                 // 0x135310  m_buffer->Restore()
    i32 ReacquireBuffer();         // 0x135340  callback / owner reacquire (extern)
    i32 StopAndRewind();           // 0x135380  Stop + SetCurrentPosition(0)
    i32 IsPlaying();               // 0x1353f0  GetStatus & DSBSTATUS_PLAYING
    i32 IsLooping();               // 0x135440  GetStatus & DSBSTATUS_LOOPING
    i32 IsInHardware();            // 0x135490  GetCaps & DSBCAPS_LOCHARDWARE
    void SetField3(i32 on);        // 0x135510  toggle the +0x14 play-flag bit 0 (looping)
    i32 SetVolume(i32 vol);        // 0x135560  SetVolume (caps DSBCAPS_CTRLVOLUME)
    i32 SetVolumeByIndex(i32 idx); // 0x1355c0  SetVolume(g_volumeTable[idx])
    i32 GetVolume();               // 0x1355f0  GetVolume
    i32 GetVolumePercent();        // 0x135640  GetVolume -> percent (0x135110)
    i32 GetPanPercent();           // 0x135840  GetPan -> signed percent (0x135110)
    i32 CloneAndPlay(i32 key, i32 mode, i32 slot); // 0x135660  reap + spawn a voice
    i32 SetPan(i32 pan);                           // 0x135740  SetPan (caps DSBCAPS_CTRLPAN)
    i32 SetPanByIndex(i32 idx);                    // 0x1357a0  SetPan(+/-g_panTable[idx]) by sign
    i32 GetPan();                                  // 0x1357f0  GetPan
    i32 SetFrequency(u32 freq); // 0x135880  SetFrequency (caps DSBCAPS_CTRLFREQUENCY)
    i32 SetField2(i32 pct);     // 0x135920  freq-percent + duration recompute
    void ComputeDuration();     // 0x1359a0  m_durationMs = m_sampleCount*1000/m_sampleRate
    i32 Unlock(void* p1, u32 n1, void* p2, u32 n2);         // 0x1359c0
    i32 GetCurrentPosition(u32* play, u32* write);          // 0x135a20
    i32 SetCurrentPosition(u32 pos);                        // 0x135a70
    i32 GetFormat(void* fmt, u32 size, u32* written);       // 0x135ac0
    i32 LoadFromFile(FILE* fp, u32 bytes, i32 offset);      // 0x135e10  fseek+Lock+fread+Unlock
    i32 LockConvert(void* src, u32 lockBytes, u32 convert); // 0x135f40
    i32 Play();                                             // 0x136270  Play + reacquire-retry
    i32 ApplyAndPlay(i32 vol, i32 pan, i32 freq, i32 d);    // 0x136300  apply params + play
    i32 Lock(
        u32 off,
        u32 bytes,
        void** p1,
        u32* n1,
        void** p2,
        u32* n2,
        u32 flags
    ); // 0x136370  Lock + reacquire-on-DSERR_BUFFERLOST retry

    static void GetErrorString(char* file, i32 line, i32 hr); // 0x138150

    // --- layout (per-buffer wrapper base, size 0x58) --------------------------
    // vptr @ +0x00 (implicit, from the virtual dtor); the first real field is +0x04.
    // +0x04 is the device buffer-list link: when the concrete leaf (DSoundCloneInst)
    // hangs in SoundDevice::m_bufferList, this DSoundLink is the biased element+4 the
    // list threads (SoundDevice::Shutdown/StopAll/CreateBuffer/RemoveBuffer). Set by
    // the list helpers, not the ctor.
    DSoundLink m_link;            // +0x04  device buffer-list link {next@+4, prev@+8}
    IDirectSoundBuffer* m_buffer; // +0x0c  the held sound buffer
    SoundDevice* m_owner;         // +0x10  owning device back-pointer
    u32 m_playFlags;              // +0x14  Play/looping flags (bit 0 = loop)
    u32 m_freq;                   // +0x18  cached frequency (GetFrequency)
    i32 m_pan;                    // +0x1c  cached pan (GetPan)
    i32 m_volume;                 // +0x20  cached volume (GetVolume)
    u32 m_setFreq;                // +0x24  cached SetFrequency value
    u32 m_durationMs;             // +0x28  duration (ComputeDuration)
    u32 m_sampleCount;            // +0x2c  sample count (set by the clone ctor)
    // +0x30  per-buffer reacquire callback (__cdecl fn-ptr taking (this, ctx)); a
    // pointer is 4 bytes, so this is layout-identical to the raw i32 slot.
    i32(__cdecl* m_reacquireCb)(DirectSoundMgr*, i32);
    i32 m_reacquireCtx;    // +0x34  per-buffer reacquire callback context
    i32 m_rateBase;        // +0x38  SetField2 percent base
    u32 m_sampleRate;      // +0x3c  sample rate (SetField2 sets it; ComputeDuration divides by it)
    u32 m_caps;            // +0x40  buffer capability flags (DSBCAPS_CTRLFREQUENCY/PAN/VOLUME)
    CloneNode m_cloneNode; // +0x44  clone-list node by which this buffer hangs in a parent's list
    i32 m_playKey;         // +0x50  clone play key
    DirectSoundMgr* m_reacquireOwner; // +0x54  device/owner used to reacquire a lost buffer
};
SIZE(0x58); // per-buffer wrapper base (fields end at +0x58)

typedef DSoundList CloneList;

class DSoundBaseSub : public DirectSoundMgr {
public:
    DSoundBaseSub(IDirectSoundBuffer* buf, SoundDevice* owner); // 0x136230
    // Clone/dup ctor 0x136180: 2-arg ctor + records source (m_reacquireOwner), copies
    // its sample/reacquire/rate block, recomputes duration.
    DSoundBaseSub(
        IDirectSoundBuffer* buf,
        SoundDevice* owner,
        DirectSoundMgr* original
    );                                 // 0x136180
    virtual ~DSoundBaseSub() OVERRIDE; // 0x136260  base-subobject dtor (vptr reset + chain)
};
SIZE(0x58); // clone alloc: Clone() news 0x58 (RezAlloc(0x58))

class DSoundCloneInst : public DSoundBaseSub {
public:
    DSoundCloneInst(IDirectSoundBuffer* buf, SoundDevice* owner); // 0x135b10
    virtual ~DSoundCloneInst() OVERRIDE;                          // 0x135bb0  clone-drain dtor

    DirectSoundMgr* Clone(i32 a);            // 0x135c20  new a clone, dup the buffer, link it
    void RemoveClone(DirectSoundMgr* clone); // 0x135d20  release + unlink + delete one clone
    void StopAllClones();                    // 0x136150  StopAndRewind each clone
    // The UI sound-cue play path (the ex "DSoundCloneInst" view of THIS class - its
    // Create @0x135c20 was Clone, its m_58 SBList was m_cloneList, its m_10
    // "CStatusBarSurface" was m_owner (SoundDevice, +0x78 = m_initialized); the
    // "DirectSoundMgr" pooled items are the clones, i.e. DirectSoundMgr):
    DirectSoundMgr* GetItem(); // 0x135d70  pull a keyed, non-playing pooled clone (or mint one)
    i32 ConfigureItem(i32 vol, i32 pan, i32 freqPct, i32 loop); // 0x1360d0  set params + Play
    // BaseInit: the 2-arg ctor (0x135b10) reached as a method so CreateBuffer's
    // RezAlloc(0x60)+construct path lowers to a reloc-masked __thiscall instead of a
    // placement-new (which cl would frame differently). Same address as the ctor above;
    // declared-only here so the call reloc-masks.
    void BaseInit(IDirectSoundBuffer* buf, SoundDevice* owner); // 0x135b10 (== ctor)

    CloneList m_cloneList; // +0x58  clone/child list {head@+0x58, tail@+0x5c}
};
SIZE(0x60); // buffer leaf: CreateBuffer RezAlloc(0x60)


// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
struct ParseFmt; // the RIFF fmt-chunk view (def in DirectSoundMgr.cpp)
extern "C" i32 ParseWaveChunks(void* riff, ParseFmt* out, void** dataOut, u32* sizeOut);
extern const char s_rb[];

extern "C" i32 ConvertVolumeToPercent(i32 v); // 0x135110 (C linkage carrier)

extern const double c_volScale;
extern const double c_volNum;
extern const double c_powExp;
extern const double c_acosNorm;
extern i32 g_panTable[];
#endif // DSNDMGR_DIRECTSOUNDMGR_H
