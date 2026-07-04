// StreamFeeder.h - the streaming feeder/pump sub-object embedded at
// StreamVoice+0x6c (Dsndmgr module, C:\Proj\Dsndmgr\DSndMgSR.CPP, retail vftable
// 0x5ef6f0). The trace tagged this class "Timer_1380d0" after its Tick pump
// (0x1380d0), but it is the StreamFeeder referenced by SoundStream::OpenStream
// (feeder->FeederStart / FeederReset / window seed) - see SoundStream.h.
//
// The feeder owns a per-stream DirectSound buffer wrapper (m_buffer, a
// DirectSoundMgr it Lock/Unlock-fills) and is armed with a data window
// (+0x2c..+0x40) over the source. Each Tick it copies the window into the
// secondary buffer (the circular Lock-fill at FillBuffer 0x137f30), wrapping +
// padding the tail with the silence byte (+0x24). Field names are placeholders;
// only OFFSETS + the emitted code bytes are load-bearing.
#ifndef DSNDMGR_STREAMFEEDER_H
#define DSNDMGR_STREAMFEEDER_H

#include <rva.h>

#include <Dsndmgr/WaveFormatX.h> // WAVEFORMATEX-shaped PCM header (FeederStart)
#include <Gruntz/ParseSource.h>  // the positioned byte-reader the feeder pulls from (m_source)

// The feeder's owner (m_owner): the SoundDevice (base of SoundStream) that creates
// the streaming DirectSound buffer (CreateBuffer 0x1366f0) and reaps it
// (RemoveBuffer 0x136d80). Full definition included in StreamFeeder.cpp; its
// methods are reloc-masked __thiscall calls.
class SoundDevice;

// The feeder's per-stream buffer wrapper (m_buffer): a DirectSoundMgr the feeder
// Lock/Unlock-fills + Stop/Pause/Play-drives. Full definition included in
// StreamFeeder.cpp; its methods are reloc-masked __thiscall calls.
class DirectSoundMgr;

// The streaming feeder. ALL-VTABLES phase: REAL polymorphic base - cl auto-emits
// ??_7StreamFeeder@@6B@ (0x5ef6f0) and auto-stamps the vptr in the ctor (0x137cd0).
// The 3 slots are declared-only virtuals (bodies external / overridden by the
// derived voice-feeder): slot 0 Feed (retail base = __purecall; kept non-pure so
// the class stays concrete/embeddable), slot 1 FeedData (0x137e10), slot 2 OnDrain
// (0x137e20). The voice's embedded feeder overrides slot 0 with CopyWindow (0x137380).
struct StreamFeeder {
    virtual i32
    Feed(void* dst1, u32 n1, u32* got1, void* dst2, u32 n2, u32* got2); // [0] feed-two-regions
    virtual i32 FeedData();                                             // [1] 0x137e10
    virtual void OnDrain();                                             // [2] 0x137e20

    // vptr @ +0x00 (implicit); first real field at +0x04.
    SoundDevice* m_owner;     // +0x04  owner (SoundStream, via its SoundDevice base)
    DirectSoundMgr* m_buffer; // +0x08  per-stream DirectSound buffer wrapper
    u32 m_bufferCursor;       // +0x0c  read cursor into the buffer (write phase)
    u32 m_bufferLength;       // +0x10  loop/buffer length
    u32 m_format;             // +0x14  format / flags (FeederStart arg4)
    u32 m_armed;              // +0x18  armed flag
    u32 m_drained;            // +0x1c  drained flag
    u32 m_pendingBytes;       // +0x20  pending-bytes accumulator
    u8 m_silenceByte;         // +0x24  silence byte (0x80 for 8-bit PCM, else 0)
    u32 m_lastTickMs;         // +0x28  last Tick time (u8 above self-aligns to +0x28)
    CParseSource* m_source;   // +0x2c  source back-pointer (window)
    u32 m_loop;               // +0x30  loop flag
    u32 m_sourceOffset;       // +0x34  running source offset
    u32 m_windowStart;        // +0x38  window start offset
    u32 m_windowLength;       // +0x3c  window length
    u32 m_windowEnd;          // +0x40  window end (start+length)

    i32 SeedWindow(CParseSource* src, u32 off, u32 len); // 0x137340
    StreamFeeder();                                      // 0x137cd0
    void Cleanup();                                      // 0x137cf0  (dtor body)
    i32 FeederStart(
        SoundDevice* owner,
        WaveFormatX* fmt,
        u32 len,
        u32 format,
        DirectSoundMgr* buf,
        i32 tickArg
    );                                       // 0x137d10
    void FeederReset(i32 doStop);            // 0x137dc0
    i32 Resume();                            // 0x137ed0
    i32 Pause();                             // 0x137f00
    i32 FillBuffer(u32 writePos, u32 bytes); // 0x137f30

    // Tick (0x1380d0): sibling pump, external to this TU - reloc-masked.
    i32 TickPump(i32 now);
};
SIZE(StreamFeeder, 0x44);       // embedded feeder sub-object (StreamVoice+0x6c..0xb0)
VTBL(StreamFeeder, 0x001ef6f0); // cl-emitted ??_7StreamFeeder@@6B@ (3-slot base)

// The DERIVED per-voice feeder (retail vtable 0x5ef6e0) embedded at StreamVoice+0x6c.
// ALL-VTABLES phase: a real StreamFeeder-derived override so cl auto-emits
// ??_7StreamVoiceFeeder@@6B@ (0x5ef6e0) via base-then-derived member construction
// (base ctor stamps 0x5ef6f0, then the derived vptr 0x5ef6e0). Adds no fields (size 0x44).
// Overrides: slot 0 Feed = CopyWindow (0x137380); slots 1/2 (0x137490 / 0x1374b0)
// are declared-only overrides (bodies external).
struct StreamVoiceFeeder : StreamFeeder {
    StreamVoiceFeeder() {} // empty: base ctor stamps 0x5ef6f0, cl then stamps 0x5ef6e0
    virtual i32 Feed(void* dst1, u32 n1, u32* got1, void* dst2, u32 n2, u32* got2)
        OVERRIDE;                    // [0] 0x137380
    virtual i32 FeedData() OVERRIDE; // [1] 0x137490
    virtual void OnDrain() OVERRIDE; // [2] 0x1374b0
};
SIZE(StreamVoiceFeeder, 0x44);       // derived feeder; no added fields
VTBL(StreamVoiceFeeder, 0x001ef6e0); // cl-emitted ??_7StreamVoiceFeeder@@6B@ (derived override)

#endif // DSNDMGR_STREAMFEEDER_H
