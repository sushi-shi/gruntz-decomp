// StreamFeeder.h - the streaming feeder/pump sub-object embedded at
// StreamVoice+0x6c (Dsndmgr module, C:\Proj\Dsndmgr\DSndMgSR.CPP, retail vftable
// 0x5ef6f0). The trace tagged this class "Timer_1380d0" after its Tick pump
// (0x1380d0), but it is the StreamFeeder referenced by SoundStream::OpenStream
// (feeder->FeederStart / FeederReset / window seed) - see SoundStream.h.
//
// The feeder owns a per-stream DirectSound buffer wrapper (m_buffer, a
// DirectSoundMgr-derived voice it Lock/Unlock-fills) and is armed with a data
// window (+0x2c..+0x40) over the source. Each Tick it copies the window into the
// secondary buffer (the circular Lock-fill at FillBuffer 0x137f30), wrapping +
// padding the tail with the silence byte (+0x24). Field names are placeholders;
// only OFFSETS + the emitted code bytes are load-bearing.
#ifndef DSNDMGR_STREAMFEEDER_H
#define DSNDMGR_STREAMFEEDER_H

#include <rva.h>

// The streaming source reader the feeder pulls window bytes from (the same
// polymorphic reader SoundStream.h models as StreamSource; declared here so this
// TU is self-contained - SoundStream.h's wider view stays in its own TU). Read
// (0x139af0) returns the byte count actually read at `at` (-1 = current cursor).
struct FeederSource {
    i32 Read(void* buf, i32 n, i32 at); // 0x139af0
};
SIZE_UNKNOWN(FeederSource); // thin reader view (method-only)

// The feeder's owner (m_owner): a SoundDevice/SoundStream-family object that creates
// the streaming DirectSound buffer (0x1366f0) and reaps it (RemoveBuffer
// 0x136d80). Opaque here - its methods are reloc-masked __thiscall calls.
struct FeederOwner {
    // 0x1366f0 - build a streaming secondary buffer from the PCM format + size,
    // return the buffer wrapper (or 0). Caller-side: ecx=owner, (fmt, bytes, flags).
    void* CreateStreamBuf(void* fmt, u32 bytes, u32 flags);
    // 0x136d80 - reap voices + release the COM buffer + unlink one wrapper.
    void RemoveBuffer(void* buf);
};
SIZE_UNKNOWN(FeederOwner); // opaque owner view (method-only)

// The feeder's per-stream buffer wrapper (m_buffer): a DirectSoundMgr-derived voice
// the feeder Lock/Unlock-fills + Stop/Pause/Resume-drives. Opaque; reloc-masked
// __thiscall calls. Offsets/sizes match DirectSoundMgr but the feeder only ever
// calls these methods, so a thin view is matching-neutral.
struct FeederBuf {
    i32 Lock(u32 off, u32 bytes, void** p1, u32* n1, void** p2, u32* n2,
             u32 flags);                            // 0x136370
    i32 Unlock(void* p1, u32 n1, void* p2, u32 n2); // 0x1359c0
    i32 StopAndRewind();                            // 0x135380
    i32 IsPlaying();                                // 0x136270  (returns play-state)
    i32 Resume(i32 flag);                           // 0x135510  (resume/restart playback)
    i32 Tick(i32 now); // 0x135a70  ... actually SetCurrentPosition pump
};
SIZE_UNKNOWN(FeederBuf); // thin DirectSoundMgr-buffer view (method-only)

// The streaming feeder. Its own vftable (0x5ef6f0) is restamped by the ctor +
// dtor (a transitional reloc-masked DIR32 store: slot 0 = scalar-deleting dtor
// 0x11fec0, slot 1 = FeedData 0x137e10, slot 2 = OnDrain 0x137e20 - all external,
// so the class stays non-polymorphic and the compiler emits no vtable).
struct StreamFeeder {
    void* m_vtbl;         // +0x00  (retail vtable 0x5ef6f0; virtuals external)
    FeederOwner* m_owner; // +0x04  owner (SoundStream/SoundDevice)
    FeederBuf* m_buffer;  // +0x08  per-stream DirectSound buffer wrapper
    u32 m_bufferCursor;   // +0x0c  read cursor into the buffer (write phase)
    u32 m_bufferLength;   // +0x10  loop/buffer length
    u32 m_format;         // +0x14  format / flags
    u32 m_armed;          // +0x18  armed flag
    u32 m_drained;        // +0x1c  drained flag
    u32 m_pendingBytes;   // +0x20  pending-bytes accumulator
    u8 m_silenceByte;     // +0x24  silence byte (0x80 for 8-bit PCM, else 0)
    char m_pad25[0x28 - 0x25];
    u32 m_lastTickMs;   // +0x28  last Tick time
    u32 m_source;       // +0x2c  source back-pointer (window)
    u32 m_loop;         // +0x30  loop flag
    u32 m_sourceOffset; // +0x34  running source offset
    u32 m_windowStart;  // +0x38  window start offset
    u32 m_windowLength; // +0x3c  window length
    u32 m_windowEnd;    // +0x40  window end (start+length)

    i32 SeedWindow(void* src, u32 off, u32 len); // 0x137340
    i32 CopyWindow(
        void* dst1,
        u32 n1,
        u32* got1,
        void* dst2,
        u32 n2,
        u32* got2
    );              // 0x137380 (slot-0 feed)
    StreamFeeder(); // 0x137cd0
    void Cleanup(); // 0x137cf0  (dtor body)
    i32 FeederStart(
        FeederOwner* owner,
        i32 arg2,
        u32 len,
        void* fmt,
        void* buf,
        i32 tickArg
    );                                       // 0x137d10
    void FeederReset(i32 doStop);            // 0x137dc0
    i32 Resume();                            // 0x137ed0
    i32 Pause();                             // 0x137f00
    i32 FillBuffer(u32 writePos, u32 bytes); // 0x137f30

    // Tick (0x1380d0): sibling pump, external to this TU - reloc-masked.
    i32 TickPump(i32 now);
};
SIZE(StreamFeeder, 0x44); // embedded feeder sub-object (StreamVoice+0x6c..0xb0)

#endif // DSNDMGR_STREAMFEEDER_H
