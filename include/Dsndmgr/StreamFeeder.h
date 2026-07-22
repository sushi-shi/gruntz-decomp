#ifndef DSNDMGR_STREAMFEEDER_H
#define DSNDMGR_STREAMFEEDER_H

#include <rva.h>

#include <Dsndmgr/WaveFormatX.h> // WAVEFORMATEX-shaped PCM header (FeederStart)
#include <Gruntz/ParseSource.h>  // the positioned byte-reader the feeder pulls from (m_source)

class SoundDevice;

class DirectSoundMgr;

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
    // The REAL (non-virtual - the 3-slot vtable has no dtor slot) destructor:
    // retail 0x137cf0 stamps ??_7StreamFeeder (0x5ef6f0) at entry then tears down
    // the armed buffer - a dtor body, and ~StreamVoice's EH state machine calls it
    // as the m_feeder member dtor (state 0). Was the `Cleanup()` placeholder.
    ~StreamFeeder(); // 0x137cf0
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

    // The two pump entries SoundDevice::TickSubManagers (0x137ac0) drives on the
    // voice's embedded feeder. Bodies are homed (EXACT) in ApiMiscHelpers.cpp as the
    // trace-tagged Throttle_137e30::Tick / Timer_1380d0::Tick placeholders - their
    // field maps are 1:1 THIS class (+0x08 m_buffer, +0x0c m_bufferCursor, +0x10
    // m_bufferLength, +0x28 m_lastTickMs, Work == FillBuffer 0x137f30); reloc-masked.
    // Tick (0x137e30): per-frame throttled pump - gate on +0x1c, 100ms throttle via
    // m_lastTickMs, read the buffer play position, FillBuffer-refill the consumed span.
    i32 Tick(i32 timestamp); // 0x137e30  (timestamp -1 = "now")
    // TickPump (0x1380d0): reset-and-reprime - zero the cursor, m_buffer->Prepare(0),
    // FillBuffer the whole window (TickSubManagers fires it with -1 on idle+stop).
    i32 TickPump(i32 now);
};
SIZE(0x44); // embedded feeder sub-object (StreamVoice+0x6c..0xb0)
VTBL(StreamFeeder, 0x001ef6f0); // cl-emitted ??_7StreamFeeder@@6B@ (3-slot base)

// The DERIVED per-voice feeder (retail vtable 0x5ef6e0) embedded at StreamVoice+0x6c.
// ALL-VTABLES phase: a real StreamFeeder-derived override so cl auto-emits
// ??_7StreamVoiceFeeder@@6B@ (0x5ef6e0) via base-then-derived member construction
// (base ctor stamps 0x5ef6f0, then the derived vptr 0x5ef6e0). Adds no fields (size 0x44).
// Overrides (bodies in SoundStream.cpp): slot 0 Feed = CopyWindow (0x137380);
// slot 1 FeedData = window rewind (0x137490); slot 2 OnDrain = no-op (0x1374b0).
// Its cl-generated IMPLICIT dtor (a bare tail-jmp to ~StreamFeeder, no vptr
// re-stamp) is the retail 0x1376c0 copy, @rva-symbol-bound in SoundStream.cpp.
struct StreamVoiceFeeder : StreamFeeder {
    StreamVoiceFeeder() {} // empty: base ctor stamps 0x5ef6f0, cl then stamps 0x5ef6e0
    virtual i32 Feed(void* dst1, u32 n1, u32* got1, void* dst2, u32 n2, u32* got2)
        OVERRIDE;                    // [0] 0x137380
    virtual i32 FeedData() OVERRIDE; // [1] 0x137490
    virtual void OnDrain() OVERRIDE; // [2] 0x1374b0
};
SIZE(0x44); // derived feeder; no added fields
VTBL(StreamVoiceFeeder, 0x001ef6e0); // cl-emitted ??_7StreamVoiceFeeder@@6B@ (derived override)

#endif // DSNDMGR_STREAMFEEDER_H
