// SoundStream.h - the WAP32 streaming DirectSound class (Dsndmgr module,
// C:\Proj\Dsndmgr\DSndMgSR.CPP, retail vftable 0x5ef6ec). It DERIVES from
// SoundDevice (the device manager, DSNDMGR.CPP, vtable 0x5ef6c4): same +0x14
// IDirectSound device, +0x78 "initialized" flag, per-buffer/voice sub-lists, etc.
// - the trace conflated all three (device / buffer-wrapper / stream) as
// "MinervaInner"; the three vftables prove they are distinct classes.
//
// SoundStream adds streaming-buffer creation from a parsed RIFF/WAVE source: it
// validates a WAVEFORMATEX, asks the device's IDirectSound for a secondary
// buffer, wraps it in a per-stream voice object (the 0xb0-byte
// DirectSoundMgr-derived StreamVoiceNode, ctor 0x1375b0), and threads that voice on
// its own +0x94 voice list. Field names are placeholders; only OFFSETS + the
// emitted code bytes are load-bearing.
#ifndef DSNDMGR_SOUNDSTREAM_H
#define DSNDMGR_SOUNDSTREAM_H

#include <rva.h>

#include <Dsndmgr/SoundDevice.h>
#include <Dsndmgr/SoundVoiceList.h> // DSoundLink (intrusive link) / DSoundList (the voice list)
#include <Dsndmgr/StreamFeeder.h>   // the real embedded feeder sub-object (StreamVoiceNode+0x6c)
#include <Gruntz/CParseSource.h>    // the RIFF/WAVE byte-reader (ParseWave / OpenStream)

class SoundStream;

// The per-stream voice object the creator allocates (0xb0 bytes via RezAlloc,
// ctor 0x1375b0). It is a DirectSoundMgr-derived buffer wrapper: +0x0c holds the
// IDirectSoundBuffer to release, +0x6c is the embedded StreamVoiceFeeder, and the
// standard +0x04 intrusive link biases +4 (MFC POSITION). On teardown its queued
// voices are reaped, the COM buffer released, the node unlinked, then its
// scalar-deleting destructor (vtable slot 0) runs.
//
// FLAG(shared): this is the same 0xb0 object StreamVoice.h models as `StreamVoice`
// (ctor 0x1375b0). The two headers pin complementary field views of one class; the
// orchestrator unifies them (and makes it derive DirectSoundMgr, which the +0x04
// intrusive-link overlap currently blocks).
struct StreamVoiceNode {
    virtual ~StreamVoiceNode(); // +0x00  slot 0 scalar-deleting dtor (0x5ef6d8; defined externally)
    DSoundLink m_link;          // +0x04  intrusive link { next@+0x04, prev@+0x08 } (POSITION+4)
    IDirectSoundBufferZ* m_buffer; // +0x0c  the IDirectSoundBuffer to release
    char m_pad10[0x28 - 0x10];     // DirectSoundMgr base region (modeled in StreamVoice.h)
    u32 m_durationMs;              // +0x28  duration-ms (= m_byteLength*1000/m_avgBytesDiv)
    u32 m_byteLength;              // +0x2c  byte length
    char m_pad30[0x38 - 0x30];
    u32 m_avgBytes;    // +0x38  avg-bytes-per-sec
    u32 m_avgBytesDiv; // +0x3c  avg-bytes-per-sec (divisor)
    char m_pad40[0x6c - 0x40];
    StreamVoiceFeeder m_feeder; // +0x6c  embedded streaming feeder sub-object

    // ctor 0x1375b0(IDirectSoundBuffer* buf, SoundStream* owner, int a, int b).
    StreamVoiceNode(IDirectSoundBufferZ* buf, SoundStream* owner, i32 a, i32 b);
    void ComputeDuration(); // 0x1359a0  m_durationMs = m_byteLength*1000/m_avgBytesDiv
};
SIZE(StreamVoiceNode, 0xb0); // 0xb0 bytes via RezAlloc (ctor 0x1375b0)

class SoundStream : public SoundDevice {
public:
    SoundStream(); // 0x1376d0  base ctor + zero the voice list + stamp 0x5ef6ec
    virtual ~SoundStream()
        OVERRIDE; // 0x137710  reset vptr (0x5ef6ec) then ~SoundDevice; cl auto-emits
                  // the ??_G scalar-deleting dtor at 0x1376f0
    StreamVoiceNode* CreateStreamBuffer(WaveFormatX* fmt, u32 bytes, i32 a, i32 b, i32 c);
    // 0x137780
    StreamVoiceNode* OpenStream(CParseSource* src, i32 p1, i32 p2, i32 p3, i32 p4, i32 p5);
    // 0x137900
    void DestroyVoice(StreamVoiceNode* voice); // 0x1379d0
    // Full free: reap (DestroyVoice) every voice off m_voices, then the base
    // SoundDevice::Shutdown. Called from CDDrawSurfaceMgr::FreeContext.
    void Free(); // 0x137740
    // Stop streaming: Pause each voice's embedded feeder (+0x6c), then the base
    // SoundDevice::StopAll. The wide pause/reset path (menu/level teardown).
    void Stop(); // 0x137a80
    i32 ParseWave(
        CParseSource* src,
        WaveFormatX* fmtBuf,
        u32* outDataOff,
        u32* outDataLen
    ); // 0x137b70

    // SoundStream's own per-stream voice list {head@+0x94, tail@+0x98}. FLAG(shared):
    // the base SoundDevice ends at +0x94 (its ctor 0x136440 zeroes only through +0x90;
    // it never touches +0x94), so this list is entirely the derived class's.
    DSoundList m_voices; // +0x94  owned per-stream voice list
};
SIZE(SoundStream, 0x9c);       // 0x94 SoundDevice base + DSoundList m_voices (8 B)
VTBL(SoundStream, 0x001ef6ec); // cl-emitted ??_7SoundStream@@6B@ (virtual dtor override)

#endif // DSNDMGR_SOUNDSTREAM_H
