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

#include <Ints.h>

#include <Dsndmgr/DirectSoundMgr.h>

// WAVEFORMATEX as the Dsndmgr validators read it: wFormatTag (PCM == 1), then the
// 16-byte PCM tail copied verbatim into the DSBUFFERDESC.lpwfxFormat scratch.
struct WaveFormatX {
    u16 wFormatTag;      // +0x00  (== 1: PCM)
    u16 nChannels;       // +0x02
    u32 nSamplesPerSec;  // +0x04
    u32 nAvgBytesPerSec; // +0x08
    u16 nBlockAlign;     // +0x0c
    u16 wBitsPerSample;  // +0x0e
    u16 cbSize;          // +0x10
};

// One owned sound-buffer wrapper as the device sees it in its +0x04 collection.
// It is a DirectSoundMgr (buffer wrapper) whose +0x04 word doubles as the
// intrusive forward link; the stored link value points 4 bytes past the next
// node (MFC POSITION bias), so node = (link - 4). The +0x0c slot holds the
// IDirectSoundBuffer to release.
struct SoundBuf {
    void* m_vtbl;                 // +0x00  (DirectSoundMgr buffer vtable)
    SoundBuf* m_link;             // +0x04  next, biased +4 (POSITION)
    void* m_pad08;                // +0x08
    IDirectSoundBufferZ* m_buf0c; // +0x0c  the IDirectSoundBuffer to release
    char m_pad10[0x14 - 0x10];

    i32 StopAndRewind();  // 0x135380  (buffer method)
    void StopAllClones(); // 0x136150  (buffer method)
};

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

class SoundDevice {
public:
    ~SoundDevice();                    // 0x136500  /GX EH destructor (vtable 0x5ef6c4) -> Shutdown
    void Shutdown();                   // 0x136690  release every owned buffer, primary, device
    void RemoveBuffer(SoundBuf* node); // 0x136d80  reap voices + release + unlink one buffer
    void StopAll();                    // 0x136de0  StopAndRewind+StopAllClones over the buffer list
    i32 FreeSamples(); // 0x136ed0  free + unlink every cached sample in the +0x0c list
    i32 SetPrimaryFormat(void* fmt); // 0x1371a0  CreatePrimaryBuffer + primary SetFormat
    i32 CreatePrimaryBuffer();       // 0x137260  (extern, defined elsewhere)
    DirectSoundMgr* CreateBuffer(
        WaveFormatX* fmt,
        u32 bytes,
        u32 flags
    );                                             // 0x1366f0  CreateSoundBuffer + wrap
    DirectSoundMgr* Acquire(void* riff, u32, u32); // 0x136910  parse RIFF + CreateBuffer + load

    // --- layout ---------------------------------------------------------------
    void* m_vtbl;        // +0x00
    SoundBuf* m_04_head; // +0x04  head of the owned-buffer list (biased +4)
    char m_pad08[0x0c - 0x08];
    void* m_0c; // +0x0c  voice/channel sub-list head (per-buffer remove)
    char m_pad10[0x14 - 0x10];
    IDirectSoundZ* m_14; // +0x14  the IDirectSound device
    char m_pad18[0x78 - 0x18];
    i32 m_78; // +0x78  "initialized" flag
    char m_pad7c[0x84 - 0x7c];
    IDirectSoundBufferZ* m_84; // +0x84  primary buffer
    char m_pad88[0x90 - 0x88];
    i32 m_90;   // +0x90
    void* m_94; // +0x94  cached-sample list/map head
};

#endif // DSNDMGR_SOUNDDEVICE_H
