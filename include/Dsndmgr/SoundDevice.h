// SoundDevice.h - the WAP32 DirectSound *device* manager (Dsndmgr module,
// C:\Proj\Dsndmgr\DSNDMGR.CPP). This is the higher-level class that OWNS the
// per-buffer DirectSoundMgr wrappers (see DirectSoundMgr.h): it holds the
// IDirectSound device (+0x14), the primary buffer (+0x84), an intrusive list of
// owned sound-buffer wrappers (head at +0x04, each wrapper chained through its
// own +0x04 link, the stored pointer biased +4 - an MFC CTypedPtrList POSITION),
// a voice/channel sub-list (+0x0c), an "initialized" flag (+0x78), and an MFC
// list of cached samples (+0x90/+0x94).
//
// Its retail vftable is 0x5ef6c4 (the *device* class), distinct from the buffer
// wrapper's 0x5ef6b8. Trace conflated it with DirectSoundMgr ("MinervaInner");
// the two vtables prove they are separate classes. Field names are placeholders
// (m_<hexoffset>); only OFFSETS + emitted code bytes are load-bearing.
#ifndef DSNDMGR_SOUNDDEVICE_H
#define DSNDMGR_SOUNDDEVICE_H

#include <rva.h>

#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/WaveFormatX.h>

// One owned sound-buffer wrapper as the device sees it in its +0x04 collection.
// It is a DirectSoundMgr (buffer wrapper) whose +0x04 word doubles as the
// intrusive forward link; the stored link value points 4 bytes past the next
// node (MFC POSITION bias), so node = (link - 4). The +0x0c slot holds the
// IDirectSoundBuffer to release.
struct SoundBuf {
    virtual void Slot0();         // +0x00  vptr slot (DirectSoundMgr buffer vtable; declared-only)
    SoundBuf* m_link;             // +0x04  next, biased +4 (POSITION)
    void* m_pad08;                // +0x08
    IDirectSoundBufferZ* m_buf0c; // +0x0c  the IDirectSoundBuffer to release
    char m_pad10[0x14 - 0x10];

    i32 StopAndRewind();  // 0x135380  (buffer method)
    void StopAllClones(); // 0x136150  (buffer method)
};
SIZE_UNKNOWN(SoundBuf); // partial DirectSoundMgr-buffer view (only +0x00..+0x14 pinned)

// One cached sample/resource node hanging off the device's +0x0c list. It is a
// polymorphic resource object (vtable, slot 1 @ +0x04 = a "free" virtual) whose
// +0x04 word doubles as the intrusive forward link (the same MFC POSITION +4
// bias as SoundBuf). On free its vptr is restamped to the abstract base
// (0x5ef6c8, a __purecall vtable) and the node is released through _RezFree.
struct SoundSample {
    virtual void Slot0(); // +0x00  slot 0 (unused here)
    virtual void Free();  // +0x04  slot 1 -> call [vtbl+4]
    SoundSample* m_link;  // +0x04  next, biased +4 (POSITION); overlays after vptr
};
SIZE_UNKNOWN(SoundSample); // cached-sample node view (real node is larger)

class SoundDevice {
public:
    SoundDevice();              // 0x136440  /GX EH base ctor (was the Ghidra placeholder
                                // "UnknownSalazar"): zero the two intrusive list members,
                                // stamp the device vptr, BuildVolumeTable, zero the rest.
    void* ScalarDtor(i32 flag); // 0x1364c0  ??_G vtable slot-0 scalar-deleting dtor:
                                // ~SoundDevice then (flag&1) operator delete; returns this.
    virtual ~SoundDevice();     // 0x136500  /GX EH destructor (vtable 0x5ef6c4) -> Shutdown.
                                // ALL-VTABLES phase: virtual so cl auto-emits ??_7SoundDevice
                                // @@6B@ (0x5ef6c4) + auto-stamps/resets the vptr.
    void Shutdown();            // 0x136690  release every owned buffer, primary, device
    void RemoveBuffer(SoundBuf* node); // 0x136d80  reap voices + release + unlink one buffer
    void StopAll();                    // 0x136de0  StopAndRewind+StopAllClones over the buffer list
    i32 FreeSamples(); // 0x136ed0  free + unlink every cached sample in the +0x0c list
    i32 SetPrimaryFormat(void* fmt); // 0x1371a0  CreatePrimaryBuffer + primary SetFormat
    i32 StartPrimary_137200();       // 0x137200  (extern) reads +0x78/+0x84, CreatePrimaryBuffer
    i32 CreatePrimaryBuffer();       // 0x137260  (extern, defined elsewhere)
    DirectSoundMgr* CreateBuffer(
        WaveFormatX* fmt,
        u32 bytes,
        u32 flags
    ); // 0x1366f0  CreateSoundBuffer + wrap
    DirectSoundMgr*
    AcquireFile(char* path, u32, u32);             // 0x136860  fopen+fread whole file -> Acquire
    DirectSoundMgr* Acquire(void* riff, u32, u32); // 0x136910  parse RIFF + CreateBuffer + load
    i32 ValidateRestore(
        DirectSoundMgr* buf,
        WaveFormatX* fmt,
        u32 size
    ); // 0x136ab0  gate + PCM check + Restore(buf)
    i32 ReloadRiff(
        DirectSoundMgr* buf,
        void* riff,
        u32 a3
    ); // 0x136bd0  re-parse RIFF, optionally downconvert, into an existing buffer

    // The volume->attenuation curve (DSNDMGR.CPP): map a 0..100 volume to a DSound
    // hundredths-of-dB attenuation via an acos/pow transfer (static, x87).
    static i32 VolumeToAttenuation(i32 value); // 0x1350b0  (was getLookupTableValue)
    static void BuildVolumeTable();            // 0x1351a0  fill g_volumeTable[0..100]

    // --- layout ---------------------------------------------------------------
    // vptr @ +0x00 (implicit, from the virtual dtor); first real field at +0x04.
    SoundBuf* m_bufferHead;  // +0x04  owned-buffer list head (biased +4)
    void* m_bufferTail;      // +0x08  owned-buffer list tail
    void* m_voiceHead;       // +0x0c  voice/channel sub-list head (per-buffer remove)
    void* m_voiceTail;       // +0x10  voice/channel sub-list tail
    IDirectSoundZ* m_device; // +0x14  the IDirectSound device
    char m_pad18[0x78 - 0x18];
    i32 m_initialized; // +0x78  "initialized" flag
    char m_pad7c[0x80 - 0x7c];
    i32 m_80;                             // +0x80
    IDirectSoundBufferZ* m_primaryBuffer; // +0x84  primary buffer
    i32 m_coopLevel;                      // +0x88
    i32 m_bufferFlags;                    // +0x8c
    i32 m_force8Bit;                      // +0x90
    void* m_94;                           // +0x94  cached-sample list/map head
};
SIZE(SoundDevice, 0x98);       // device base (SoundStream's m_98 is the first past-base member)
VTBL(SoundDevice, 0x001ef6c4); // cl-emitted ??_7SoundDevice@@6B@ (virtual dtor)

#endif // DSNDMGR_SOUNDDEVICE_H
