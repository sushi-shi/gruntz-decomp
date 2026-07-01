// SoundStream.h - the WAP32 streaming DirectSound class (Dsndmgr module,
// C:\Proj\Dsndmgr\DSndMgSR.CPP, retail vftable 0x5ef6ec). It DERIVES from
// SoundDevice (the device manager, DSNDMGR.CPP, vtable 0x5ef6c4): same +0x14
// IDirectSound device, +0x78 "initialized" flag, +0x94 owned-instance list
// head, etc. - the trace conflated all three (device / buffer-wrapper / stream)
// as "MinervaInner"; the three vftables prove they are distinct classes.
//
// SoundStream adds streaming-buffer creation from a parsed RIFF/WAVE source: it
// validates a WAVEFORMATEX, asks the device's IDirectSound for a secondary
// buffer, wraps it in a per-stream voice object (the 0xb0-byte
// DirectSoundMgr-derived StreamVoiceNode, ctor 0x1375b0), and threads that voice on
// the inherited +0x94 list. Field names are placeholders; only OFFSETS + the
// emitted code bytes are load-bearing.
#ifndef DSNDMGR_SOUNDSTREAM_H
#define DSNDMGR_SOUNDSTREAM_H

#include <rva.h>

#include <Dsndmgr/SoundDevice.h>

struct StreamVoiceNode;
class SoundStream;

// One streaming source as the parser/creator sees it: a polymorphic reader
// object (vtable @ +0x00) whose +0x18 word is the current read cursor, whose
// +0x0c word is the source length and whose +0x18 doubles after a Read.
// Seek(0x139ae0) sets +0x18; Read(0x139af0) reads `n` bytes at an explicit (or
// -1 = current) cursor. The trace tagged this reader ClassUnknown_85.
struct StreamSource {
    void* m_vtbl; // +0x00
    char m_pad04[0x0c - 0x04];
    u32 m_0c; // +0x0c  source length
    char m_pad10[0x18 - 0x10];
    u32 m_18; // +0x18  read cursor / file position

    i32 Seek(i32 pos);                  // 0x139ae0  m_18 = pos
    i32 Read(void* buf, i32 n, i32 at); // 0x139af0  read n bytes (at == -1: current)
};
SIZE_UNKNOWN(StreamSource); // partial reader view (only +0x0c/+0x18 pinned)

// The streaming feeder sub-object embedded at StreamVoiceNode+0x6c (a Timer-ish
// pump the trace tagged Timer_1380d0). Its +0x18 word is a cursor that
// FeederReset(0x137dc0) zeroes; FeederStart(0x137d10) arms the pump. The creator
// pre-seeds its data window (+0x2c..+0x3c) before arming.
struct StreamFeederView {
    char m_pad00[0x18];
    u32 m_18; // +0x18  cursor
    char m_pad1c[0x2c - 0x1c];
    u32 m_2c; // +0x2c  source back-pointer
    u32 m_30; // +0x30
    u32 m_34; // +0x34
    u32 m_38; // +0x38  data length
    u32 m_3c; // +0x3c  data back-pointer
    char m_pad40[0x44 - 0x40];

    i32 FeederStart(SoundStream* owner, void* fmt, i32 b, i32 c, StreamVoiceNode* voice, i32 d);
    // 0x137d10
    void FeederReset(i32 flag); // 0x137dc0
};

// The per-stream voice object the creator allocates (0xb0 bytes via RezAlloc,
// ctor 0x1375b0). It is a DirectSoundMgr-derived buffer wrapper: +0x0c holds the
// IDirectSoundBuffer to release, +0x6c is the embedded StreamFeederView, and the
// standard +0x04 intrusive link biases +4 (MFC POSITION). On teardown its queued
// voices are reaped, the COM buffer released, the node unlinked, then its
// scalar-deleting destructor runs.
struct StreamVoiceNode {
    void* m_vtbl;                 // +0x00
    StreamVoiceNode* m_link;      // +0x04  next, biased +4 (POSITION)
    void* m_pad08;                // +0x08
    IDirectSoundBufferZ* m_buf0c; // +0x0c  the IDirectSoundBuffer to release
    char m_pad10[0x28 - 0x10];
    u32 m_28; // +0x28  duration-ms (= m_2c*1000/m_3c)
    u32 m_2c; // +0x2c  byte length
    char m_pad30[0x38 - 0x30];
    u32 m_38; // +0x38  avg-bytes-per-sec
    u32 m_3c; // +0x3c  avg-bytes-per-sec (divisor)
    char m_pad40[0x6c - 0x40];
    StreamFeederView m_feeder; // +0x6c  embedded streaming feeder sub-object

    // ctor 0x1375b0(IDirectSoundBuffer* buf, SoundStream* owner, int a, int b).
    StreamVoiceNode(IDirectSoundBufferZ* buf, SoundStream* owner, i32 a, i32 b);
    void ComputeDuration(); // 0x1359a0  m_28 = m_2c*1000/m_3c
};

class SoundStream : public SoundDevice {
public:
    SoundStream();              // 0x1376d0  base ctor + zero m_94/m_98 + stamp 0x5ef6ec
    void* ScalarDtor(i32 flag); // 0x1376f0  ??_G: ~SoundStream then operator delete
    ~SoundStream();             // 0x137710  restamp vptr (0x5ef6ec) then ~SoundDevice
    StreamVoiceNode* CreateStreamBuffer(WaveFormatX* fmt, u32 bytes, i32 a, i32 b, i32 c);
    // 0x137780
    StreamVoiceNode* OpenStream(StreamSource* src, i32 p1, i32 p2, i32 p3, i32 p4, i32 p5);
    // 0x137900
    void DestroyVoice(StreamVoiceNode* voice); // 0x1379d0
    i32 ParseWave(
        StreamSource* src,
        WaveFormatX* fmtBuf,
        u32* outDataOff,
        u32* outDataLen
    ); // 0x137b70

    // SoundStream's own first member past the 0x98 SoundDevice base; m_94 (the
    // ctor's other zeroed word) is the inherited SoundDevice +0x94 list head.
    i32 m_98; // +0x98
};
SIZE(SoundStream, 0x9c); // 0x98 SoundDevice base + i32 m_98

#endif // DSNDMGR_SOUNDSTREAM_H
