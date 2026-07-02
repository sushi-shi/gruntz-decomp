// StreamVoice.h - the per-stream voice wrapper (Dsndmgr module,
// C:\Proj\Dsndmgr\DSndMgSR.CPP, retail vftable 0x5ef6d8). The 0xb0-byte
// DirectSoundMgr-derived buffer wrapper RezAlloc'd + constructed by
// SoundStream::CreateStreamBuffer (ctor 0x1375b0): +0x10 holds the owning
// SoundStream (m_owner), +0x6c is the embedded StreamFeeder sub-object (whose
// vptr the voice overrides to 0x5ef6e0), and the DirectSoundMgr base fields below
// +0x6c (m_3c the avg-bytes divisor) come from the shared per-buffer this-shape.
//
// Field names are placeholders; only OFFSETS + the emitted code bytes are
// load-bearing.
#ifndef DSNDMGR_STREAMVOICE_H
#define DSNDMGR_STREAMVOICE_H

#include <rva.h>

#include <Dsndmgr/StreamFeeder.h>
#include <Dsndmgr/WaveFormatX.h>

struct IDirectSoundBufferZ;
class DirectSoundMgr;

// The streaming source reader SetSource parses + arms the feeder over (the same
// polymorphic reader SoundStream.h models as StreamSource). Opaque here - the
// owning SoundStream's ParseWave does the reads.
struct StreamSource;

// The owning SoundStream as the voice sees it (m_owner @ +0x10): its +0x78 word
// is the "device up" flag the voice's Configure gates on, and ParseWave (0x137b70)
// is the RIFF/WAVE parser the voice asks for fmt + data extents. Opaque otherwise -
// reloc-masked __thiscall calls.
struct VoiceOwner {
    char m_pad0[0x78];
    i32 m_initialized; // +0x78  device-up flag
    i32 ParseWave(StreamSource* src, WaveFormatX* fmtBuf, u32* outOff,
                  u32* outLen); // 0x137b70
};
SIZE_UNKNOWN(VoiceOwner); // partial owning-SoundStream view (only +0x78 pinned)

// The per-stream voice. Its DirectSoundMgr base index-setters (SetVolumeByIndex
// 0x1355c0 / SetPanByIndex 0x1357a0 / SetFreqByIndex 0x135920) + the base
// init/dtor (0x135b10 / 0x135bb0) are reloc-masked __thiscall calls into the base
// run; the voice only ever calls these, so a thin own-decl view is matching-neutral.
struct StreamVoice {
    void* m_vtbl; // +0x00  (retail vtable 0x5ef6d8; virtuals external)
    char m_pad04[0x10 - 0x04];
    VoiceOwner* m_owner; // +0x10  owning SoundStream (also the base m_owner)
    char m_pad14[0x3c - 0x14];
    u32 m_3c; // +0x3c  avg-bytes-per-sec divisor (DirectSoundMgr base field)
    char m_pad40[0x60 - 0x40];
    i32 m_60; // +0x60  cached ctor arg b
    i32 m_64; // +0x64  cached ctor arg a
    i32 m_68; // +0x68  zero-init in ctor
    // +0x6c  embedded streaming feeder sub-object. ALL-VTABLES phase: held as the
    // DERIVED StreamVoiceFeeder (retail vtable 0x5ef6e0) so cl auto-constructs it
    // base-then-derived (0x5ef6f0 then 0x5ef6e0) - was a base StreamFeeder + a manual
    // `*(void**)&m_feeder = g_StreamVoiceFeederVtbl` override in the voice ctor. The
    // voice's +0x9c / +0xa8 stores land INSIDE this feeder: +0x9c = feeder+0x30 (loop
    // flag m_loop), +0xa8 = feeder+0x3c (window length m_windowLength).
    StreamVoiceFeeder m_feeder;

    // ctor 0x1375b0(IDirectSoundBuffer* buf, SoundStream* owner, int a, int b). The
    // empty mem-init list keeps the feeder default-constructed (its ctor 0x137cd0)
    // - retail orders BaseInit before that feeder ctor + the vptr restamps, an
    // ordering the EH-modeled hierarchy would reproduce (deferred, see the .cpp).
    StreamVoice(IDirectSoundBufferZ* buf, DirectSoundMgr* owner, i32 a, i32 b);
    ~StreamVoice();                                      // 0x137650
    i32 SetSource(StreamSource* src);                    // 0x1374c0
    i32 Configure(i32 vol, i32 pan, i32 freq, i32 loop); // 0x137520
    u32 ComputeRatio();                                  // 0x137590

    // DirectSoundMgr base run, reloc-masked __thiscall (defined in their own TUs).
    void BaseInit(IDirectSoundBufferZ* buf, DirectSoundMgr* owner); // 0x135b10
    void BaseDtor();                                                // 0x135bb0
    i32 SetVolumeByIndex(i32 idx);                                  // 0x1355c0
    i32 SetPanByIndex(i32 idx);                                     // 0x1357a0
    i32 SetFreqByIndex(i32 idx);                                    // 0x135920
};
SIZE(StreamVoice, 0xb0); // 0xb0 bytes via RezAlloc (ctor 0x1375b0)

#endif // DSNDMGR_STREAMVOICE_H
