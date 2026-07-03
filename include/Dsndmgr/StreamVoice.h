// StreamVoice.h - the per-stream voice wrapper (Dsndmgr module,
// C:\Proj\Dsndmgr\DSndMgSR.CPP, retail vftable 0x5ef6d8). The 0xb0-byte
// DirectSoundMgr-derived buffer wrapper (operator new(0xb0)) constructed by
// SoundStream::CreateStreamBuffer (ctor 0x1375b0): +0x10 holds the owning
// SoundStream (m_owner), +0x6c is the embedded StreamVoiceFeeder sub-object (whose
// vptr the voice overrides to 0x5ef6e0), and the DirectSoundMgr base fields below
// +0x6c (m_avgBytesPerSec the position->time divisor) come from the shared
// per-buffer this-shape.
#ifndef DSNDMGR_STREAMVOICE_H
#define DSNDMGR_STREAMVOICE_H

#include <Gruntz/CParseSource.h>
#include <rva.h>

#include <Dsndmgr/StreamFeeder.h>
#include <Dsndmgr/WaveFormatX.h>

struct IDirectSoundBufferZ;
class DirectSoundMgr;

// The streaming source reader SetSource parses + arms the feeder over (the same
// polymorphic reader SoundStream.h models as CParseSource). Opaque here - the
// owning SoundStream's ParseWave does the reads.
struct CParseSource;

// The owning SoundStream (m_owner @ +0x10) - the real Dsndmgr streaming device
// that created this voice (SoundStream::CreateStreamBuffer passes `this`); its
// +0x78 "device up" flag (inherited from SoundDevice) gates the voice's Configure,
// and ParseWave (0x137b70) is the RIFF/WAVE parser the voice asks for fmt + data
// extents. Full definition included in StreamVoice.cpp.
class SoundStream;

// The per-stream voice: a DirectSoundMgr-derived per-buffer object. IDEALLY
// `struct StreamVoice : DirectSoundMgr`, but the shared DirectSoundMgr is modeled
// as one dual-this-shape class (sizeof 0x90 spanning both the manager and buffer
// views), which would push the embedded feeder past its true +0x6c home - so the
// voice is a flat class over the per-buffer base layout and reaches the base run
// (SetVolumeByIndex 0x1355c0 / SetPanByIndex 0x1357a0 / SetFreqByIndex 0x135920 and
// the base init/dtor 0x135b10 / 0x135bb0) through its own reloc-masked __thiscall
// declarations. (FLAG for matcher-6: splitting DirectSoundMgr into a <=0x60-byte
// per-buffer base would let this derive.)
struct StreamVoice {
    // Real polymorphic voice: the lone virtual is the destructor, so cl emits a
    // 1-slot ??_7StreamVoice (slot 0 = the scalar-deleting dtor, matching retail's
    // 0x5ef6d8) and auto-stamps/auto-resets the vptr in the ctor/dtor.
    virtual ~StreamVoice(); // +0x00  0x137650  (slot 0 = scalar-deleting dtor)

    char m_pad04[0x10 - 0x04];
    SoundStream* m_owner; // +0x10  owning SoundStream (also the base m_owner)
    char m_pad14[0x3c - 0x14];
    u32 m_avgBytesPerSec; // +0x3c  position->time divisor (DirectSoundMgr base field)
    char m_pad40[0x60 - 0x40];
    i32 m_streamArgA; // +0x60  cached ctor arg a (arg3), consumed by the stream pump
    i32 m_streamArgB; // +0x64  cached ctor arg b (arg4)
    i32 m_streamPos;  // +0x68  zero-init position/state
    // +0x6c  embedded streaming feeder sub-object, held as the DERIVED
    // StreamVoiceFeeder (retail vtable 0x5ef6e0) so cl auto-constructs it
    // base-then-derived (0x5ef6f0 then 0x5ef6e0). The voice's +0x9c / +0xa8 stores
    // land INSIDE this feeder: +0x9c = feeder+0x30 (m_loop), +0xa8 = feeder+0x3c
    // (m_windowLength).
    StreamVoiceFeeder m_feeder;

    // ctor 0x1375b0(buf, owner, a, b): run the base init, cache a/b, clear the
    // position; the feeder is default-constructed (its ctor 0x137cd0) between.
    StreamVoice(IDirectSoundBufferZ* buf, DirectSoundMgr* owner, i32 a, i32 b);
    i32 SetSource(CParseSource* src);                    // 0x1374c0
    i32 Configure(i32 vol, i32 pan, i32 freq, i32 loop); // 0x137520
    u32 ComputeRatio();                                  // 0x137590

    // DirectSoundMgr base run, reloc-masked __thiscall (defined in their own TUs).
    void BaseInit(IDirectSoundBufferZ* buf, DirectSoundMgr* owner); // 0x135b10
    void BaseDtor();                                                // 0x135bb0
    i32 SetVolumeByIndex(i32 idx);                                  // 0x1355c0
    i32 SetPanByIndex(i32 idx);                                     // 0x1357a0
    i32 SetFreqByIndex(i32 idx);                                    // 0x135920
};
SIZE(StreamVoice, 0xb0); // 0xb0 bytes via operator new (ctor 0x1375b0)

#endif // DSNDMGR_STREAMVOICE_H
