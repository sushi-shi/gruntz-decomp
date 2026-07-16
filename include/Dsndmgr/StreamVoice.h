// StreamVoice.h - the per-stream voice wrapper (Dsndmgr module,
// C:\Proj\Dsndmgr\DSndMgSR.CPP, retail vftable 0x5ef6d8). The 0xb0-byte
// DSoundCloneInst-derived buffer wrapper (RezAlloc(0xb0)) constructed by
// SoundStream::CreateStreamBuffer (ctor 0x1375b0): the whole per-buffer base run
// (+0x00..+0x60) is the inherited DSoundCloneInst (DirectSoundMgr 0x58 ->
// DSoundBaseSub 0x58 -> DSoundCloneInst 0x60); the voice's own fields start at
// +0x60 (idle-policy flags), with the embedded StreamVoiceFeeder at +0x6c (whose
// vptr the voice's ctor stamps to 0x5ef6e0). ONE class: the former SoundStream.h
// StreamVoiceNode and DirectSoundMgr.cpp SubNode/SubInnerList/SubGuard views are
// folded here (wave 3).
//
// REAL DERIVATION PROOF (RTTI absent - engine lib, /GR off; proof = ctor/dtor
// chain + stamps + layout, from the retail bytes):
//   * ctor 0x1375b0: /GX frame; call 0x135b10 (the DSoundCloneInst ctor) with
//     (buf, owner); EH state 0 = base-constructed; feeder ctor 0x137cd0 at
//     this+0x6c + derived stamp 0x5ef6e0; own stamp 0x5ef6d8; fields at
//     +0x60/+0x64/+0x68.
//   * dtor 0x137650: /GX frame; stamp 0x5ef6d8; state 1 body = FeederReset(0);
//     state 0 -> call 0x137cf0 (~StreamFeeder, the compiler-run member dtor);
//     state -1 -> call 0x135bb0 (~DSoundCloneInst, the compiler-run base chain).
//   * vtable 0x5ef6d8: 1 slot (??_G @0x137630), same shape as the 1-slot
//     DirectSoundMgr/DSoundBaseSub/DSoundCloneInst tables at 0x5ef6b8/c0/bc.
#ifndef DSNDMGR_STREAMVOICE_H
#define DSNDMGR_STREAMVOICE_H

#include <Gruntz/ParseSource.h>
#include <rva.h>

#include <Dsndmgr/DirectSoundMgr.h> // DSoundCloneInst - the per-buffer base chain
#include <Dsndmgr/StreamFeeder.h>
#include <Dsndmgr/WaveFormatX.h>

struct IDirectSoundBuffer;

// The streaming source reader SetSource parses + arms the feeder over is the
// canonical CParseSource (included above); the owning SoundStream's ParseWave
// does the reads.

// The owning SoundStream (the base m_owner @ +0x10) - the real Dsndmgr streaming
// device that created this voice (SoundStream::CreateStreamBuffer passes `this`);
// its +0x78 "device up" flag (inherited from SoundDevice) gates the voice's
// Configure, and ParseWave (0x137b70) is the RIFF/WAVE parser the voice asks for
// fmt + data extents. Full definition included in StreamVoice.cpp.
class SoundStream;

// The per-stream voice: a DSoundCloneInst-derived per-buffer object. The base
// duration block is reused with byte units for a STREAM voice: the creator fills
// m_sampleCount = the data byte length, m_rateBase = m_sampleRate =
// fmt->nAvgBytesPerSec - so the base duration formula (ComputeDuration:
// m_sampleCount*1000/m_sampleRate) and ComputeRatio's position->time divide hold.
// The base m_link (+0x04) is the SoundDevice instance-list link (m_instanceHead
// threads voice+4; elemOf<> unbias), m_buffer (+0x0c) the IDirectSoundBuffer,
// m_owner (+0x10) the owning SoundStream (stored via its SoundDevice base).
struct StreamVoice : public DSoundCloneInst {
    // ctor 0x1375b0(buf, owner, a, b): chain the DSoundCloneInst base ctor
    // (0x135b10), cl constructs the embedded feeder (0x137cd0 then the 0x5ef6e0
    // derived stamp), then cache the idle-policy flags a/b and clear the active
    // latch. `owner` is the creating SoundStream (CreateStreamBuffer passes
    // `this`; upcasts to the base ctor's SoundDevice*).
    StreamVoice(IDirectSoundBuffer* buf, SoundStream* owner, i32 a, i32 b);
    // dtor 0x137650: FeederReset(0), then cl destroys m_feeder (~StreamFeeder
    // 0x137cf0) and chains the base ~DSoundCloneInst (0x135bb0), under the /GX
    // frame. Slot 0 of the 1-slot ??_7StreamVoice (0x5ef6d8) is the auto-emitted
    // ??_G scalar-deleting dtor (retail 0x137630).
    virtual ~StreamVoice() OVERRIDE; // 0x137650

    i32 SetSource(CParseSource* src);                    // 0x1374c0
    i32 Configure(i32 vol, i32 pan, i32 freq, i32 loop); // 0x137520
    u32 ComputeRatio();                                  // 0x137590

    // ctor args a/b are the idle-policy flags SoundDevice::TickSubManagers (0x137ac0)
    // polls each frame; +0x68 is zero-init then updated with the IsPlaying result.
    i32 m_stopWhenIdle;   // +0x60  ctor arg a: reprime the feeder when the buffer goes idle
    i32 m_retireWhenIdle; // +0x64  ctor arg b: RemoveSub this voice when it goes idle
    i32 m_active;         // +0x68  latched IsPlaying state (zero-init)
    // +0x6c  embedded streaming feeder sub-object, held as the DERIVED
    // StreamVoiceFeeder (retail vtable 0x5ef6e0) so cl auto-constructs it
    // base-then-derived (0x5ef6f0 then 0x5ef6e0). The voice's +0x9c / +0xa8 stores
    // land INSIDE this feeder: +0x9c = feeder+0x30 (m_loop), +0xa8 = feeder+0x3c
    // (m_windowLength).
    StreamVoiceFeeder m_feeder;
};
SIZE(StreamVoice, 0xb0); // 0x60 DSoundCloneInst base + 0xc flags + 0x44 feeder
VTBL(
    StreamVoice,
    0x001ef6d8
); // 1-slot ??_7StreamVoice (slot 0 = scalar-deleting dtor ??_G 0x137630)

#endif // DSNDMGR_STREAMVOICE_H
