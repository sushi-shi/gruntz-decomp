// SoundDevice.h - the WAP32 DirectSound *device* manager (Dsndmgr module,
// C:\Proj\Dsndmgr\DSNDMGR.CPP). This is the higher-level class that OWNS the
// per-buffer sound-buffer wrappers: it holds the IDirectSound device (+0x14),
// the primary buffer (+0x84), an intrusive list of owned sound-buffer wrappers
// (a DSoundList value sub-object at +0x04, each buffer chained through its own
// +0x04 link, the stored pointer biased +4 - an engine POSITION), a voice/channel
// sub-list (a DSoundList at +0x0c), an "initialized" flag (+0x78) and a
// per-derived instance-list head (+0x94, used by SoundStream).
//
// Its retail vftable is 0x5ef6c4 (the *device* class), distinct from the buffer
// wrapper's 0x5ef6b8. SoundDevice is never instantiated on its own - it is the
// base subobject of SoundStream (DSndMgSR.CPP, 0x5ef6ec), the concrete class;
// every device method runs on a SoundStream `this`.
#ifndef DSNDMGR_SOUNDDEVICE_H
#define DSNDMGR_SOUNDDEVICE_H

#include <rva.h>

#include <Dsndmgr/DirectSoundMgr.h> // DirectSoundMgr buffer wrapper + IDirectSound COM
#include <Dsndmgr/SoundVoiceList.h> // shared DSoundList / DSoundLink / DSoundElem / PureSoundElem
#include <Dsndmgr/WaveFormatX.h>    // WAVEFORMATEX-shaped PCM header

class SoundDevice;

// SoundBuf - one owned sound buffer, as the device's +0x04 buffer list threads
// it. It is the 0x60-byte DirectSound buffer object CreateBuffer mints (retail
// vtable 0x5ef6bc, stamped by BaseInit): slot 0 is its scalar-deleting destructor
// and its +0x04 word doubles as the intrusive forward link (biased +4, so the
// owning node is `link - 4`). BaseInit/ComputeDuration/StopAndRewind/StopAllClones
// live in DirectSoundMgr.cpp; declared no-body here so the __thiscall calls reloc-
// mask.
//
// FLAG (matcher-6): this IS the DirectSoundMgr buffer leaf (DSoundCloneInst,
// 0x5ef6bc); DirectSoundMgr's +0x04 pad is this device-list link. Kept as a local
// node view here - unifying it into DirectSoundMgr (a real DSoundLink at +0x04 +
// these seed fields) would drop the two SoundBuf<->DirectSoundMgr bridge casts in
// SoundDevice.cpp.
struct SoundBuf {
    virtual ~SoundBuf(); // +0x00  slot 0 scalar-deleting dtor (0x5ef6bc; defined externally)
    DSoundLink m_link;   // +0x04  device buffer-list link (next@+4, prev@+8)
    IDirectSoundBufferZ* m_buffer; // +0x0c  the IDirectSoundBuffer to release
    char m_reservedA[0x18 - 0x10]; // +0x10
    u32 m_formatWord;              // +0x18  wFormatTag|nChannels of the WAVEFORMATEX
    char m_reservedB[0x28 - 0x1c]; // +0x1c
    u32 m_durationMs;              // +0x28  duration-ms (set by ComputeDuration)
    u32 m_byteCount;               // +0x2c
    char m_reservedC[0x38 - 0x30]; // +0x30
    u32 m_avgBytesPerSec;          // +0x38
    u32 m_avgBytesPerSecDivisor;   // +0x3c
    char m_reservedD[0x60 - 0x40]; // +0x40

    void BaseInit(IDirectSoundBufferZ* buf, SoundDevice* owner); // 0x135b10
    void ComputeDuration();                                      // 0x1359a0
    i32 StopAndRewind();                                         // 0x135380
    void StopAllClones();                                        // 0x136150
};
SIZE(SoundBuf, 0x60); // RezAlloc(0x60) in CreateBuffer - the DirectSound buffer leaf. Its
                      // vtable (0x5ef6bc) is cl-emitted as ??_7DSoundCloneInst by
                      // DirectSoundMgr.cpp (matcher-6); no VTBL here (SoundBuf is a view -
                      // never cl-instantiated, so no ??_7SoundBuf is emitted).

// ParseFmt - the fmt-chunk descriptor ParseWaveChunks fills (its `out` param) and
// Acquire/ReloadRiff read. m_fmt points at the WAVEFORMATEX inside the RIFF blob;
// m_flags carries the parse flags (bit 0 forces an 8-bit downconvert). Its address
// escapes to the parser, so the pre-zeroed slots stay live (not constant-folded).
struct ParseFmt {
    WaveFormatX* m_fmt; // +0x00  fmt-chunk WAVEFORMATEX pointer (into the RIFF blob)
    u32 m_reservedA;    // +0x04  (zeroed by Acquire; parser output slot, unused for WAVE)
    u32 m_flags;        // +0x08  parse flags (bit 0 -> force an 8-bit downconvert)
    u32 m_reservedB;    // +0x0c
    u32 m_reservedC;    // +0x10  (zeroed by Acquire)
};
SIZE(ParseFmt, 0x14); // 5-DWORD parser scratch descriptor (address escapes)

struct SubNode; // TickSubManagers instance-list node (view; full def in DirectSoundMgr.cpp)

class SoundDevice {
public:
    SoundDevice();          // 0x136440  /GX EH base ctor (init lists, BuildVolumeTable, zero)
    virtual ~SoundDevice(); // 0x136500  /GX EH dtor (vtable 0x5ef6c4) -> Shutdown; cl emits
                            // ??_7SoundDevice@@6B@ + the ??_G scalar-deleting thunk (0x1364c0,
                            // labelled by @rva-symbol in SoundDevice.cpp).
    void Shutdown();        // 0x136690  release every owned buffer, primary, device
    void RemoveBuffer(SoundBuf* node); // 0x136d80  reap voices + release + unlink one buffer
    void StopAll();                    // 0x136de0  StopAndRewind+StopAllClones over the list
    i32 FreeSamples();                 // 0x136ed0  free + unlink every cached voice (+0x0c list)
    i32 SetPrimaryFormat(void* fmt);   // 0x1371a0  CreatePrimaryBuffer + primary SetFormat; the
                                       // fmt is an opaque WAVEFORMATEX buffer (callers pass their
                                       // own pointer type), so it stays void*.
    i32 StartPrimary();                // 0x137200  (extern) reads +0x78/+0x84, primary
    i32 CreatePrimaryBuffer();         // 0x137260  (extern, defined elsewhere)
    DirectSoundMgr* CreateBuffer(
        WaveFormatX* fmt,
        u32 bytes,
        u32 flags
    ); // 0x1366f0  CreateSoundBuffer + wrap
    DirectSoundMgr*
    AcquireFile(char* path, u32 flags, u32 reserved); // 0x136860  fopen whole file -> Acquire
    DirectSoundMgr* Acquire(void* riff, u32, u32);    // 0x136910  parse RIFF + CreateBuffer + load
    i32 ValidateRestore(
        DirectSoundMgr* buf,
        WaveFormatX* fmt,
        u32 size
    ); // 0x136ab0  gate + PCM check + Restore(buf)
    i32 ReloadRiff(
        DirectSoundMgr* buf,
        void* riff,
        u32 reserved
    ); // 0x136bd0  re-parse RIFF, optionally downconvert, into an existing buffer

    // Device bring-up (DSNDMGR.CPP; defined in DirectSoundMgr.cpp - they fall in that
    // RVA range): DirectSoundCreate + cooperative level, then lazy primary buffer.
    i32 Create(void* hwnd, u32 level, u32 flags);   // 0x136550  DirectSoundCreate + coop
    i32 ReacquireViaCallback();                     // 0x1365e0  dispatch m_reacquireProc
    i32 SetCooperativeLevel(void* hwnd, u32 level); // 0x1365f0
    // Per-tick device-list housekeeping (also defined in DirectSoundMgr.cpp).
    i32 PurgeVoiceList(i32 time);  // 0x136e20  reap finished voices from m_voiceList
    void RemoveSub(SubNode* n);    // 0x1379d0  retire one instance-list sub-object (extern)
    i32 TickSubManagers(i32 time); // 0x137ac0  tick each derived instance

    // The volume->attenuation curve (DSNDMGR.CPP): map a 0..100 volume to a DSound
    // hundredths-of-dB attenuation via an acos/pow transfer (static, x87).
    static i32 VolumeToAttenuation(i32 value); // 0x1350b0
    static void BuildVolumeTable();            // 0x1351a0  fill g_volumeTable[0..100]

    // --- layout ---------------------------------------------------------------
    // vptr @ +0x00 (implicit, from the virtual dtor); the first real field is +0x04.
    DSoundList m_bufferList; // +0x04  owned-buffer list {head@+4, tail@+8} (biased +4 links)
    DSoundList m_voiceList;  // +0x0c  voice/channel sub-list {head@+0xc, tail@+0x10}
    IDirectSoundZ* m_device; // +0x14  the IDirectSound device
    // +0x18..+0x78: unused by the device shape (the per-buffer fields DirectSoundMgr
    // uses in the same layout region; the device role never touches them).
    char m_reserved[0x78 - 0x18];
    i32 m_initialized; // +0x78  "initialized" flag (gates every op)
    i32 m_createFlag;  // +0x7c  cleared by Create; reused by PurgeVoiceList as a tick stamp
    // +0x80  reacquire callback: a pointer-to-member on the device that
    // ReacquireViaCallback (0x1365e0) tail-dispatches through. A single-inheritance
    // member-fn-ptr is 4 bytes (just the code address), so this is layout-identical
    // to the raw slot - and __thiscall by default, matching the retail dispatch.
    i32 (SoundDevice::*m_reacquireProc)();
    IDirectSoundBufferZ* m_primaryBuffer; // +0x84  primary buffer
    i32 m_coopLevel;                      // +0x88  cooperative level
    u32 m_bufferFlags;                    // +0x8c  buffer-desc flags
    i32 m_force8Bit;                      // +0x90  force-8-bit downconvert flag (Acquire reads)
    DSoundLink* m_instanceHead;           // +0x94  derived instance-list head (SoundStream)
};
SIZE(SoundDevice, 0x98);       // device base (SoundStream's first own member is at +0x98)
VTBL(SoundDevice, 0x001ef6c4); // cl-emitted ??_7SoundDevice@@6B@ (virtual dtor)

#endif // DSNDMGR_SOUNDDEVICE_H
