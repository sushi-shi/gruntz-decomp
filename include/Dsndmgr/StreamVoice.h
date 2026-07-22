#ifndef DSNDMGR_STREAMVOICE_H
#define DSNDMGR_STREAMVOICE_H

#include <Gruntz/ParseSource.h>
#include <rva.h>

#include <Dsndmgr/DirectSoundMgr.h> // DSoundCloneInst - the per-buffer base chain
#include <Dsndmgr/StreamFeeder.h>
#include <Dsndmgr/WaveFormatX.h>

struct IDirectSoundBuffer;

class SoundStream;

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
SIZE(0xb0); // 0x60 DSoundCloneInst base + 0xc flags + 0x44 feeder

#endif // DSNDMGR_STREAMVOICE_H
