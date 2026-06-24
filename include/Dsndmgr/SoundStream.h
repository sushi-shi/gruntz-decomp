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
// DirectSoundMgr-derived StreamVoice, ctor 0x1375b0), and threads that voice on
// the inherited +0x94 list. Field names are placeholders; only OFFSETS + the
// emitted code bytes are load-bearing.
#ifndef DSNDMGR_SOUNDSTREAM_H
#define DSNDMGR_SOUNDSTREAM_H

#include <Dsndmgr/SoundDevice.h>

struct StreamVoice;
class SoundStream;

// One streaming source as the parser/creator sees it: a polymorphic reader
// object (vtable @ +0x00) whose +0x18 word is the current read cursor, whose
// +0x0c word is the source length and whose +0x18 doubles after a Read.
// Seek(0x139ae0) sets +0x18; Read(0x139af0) reads `n` bytes at an explicit (or
// -1 = current) cursor. The trace tagged this reader ClassUnknown_85.
struct StreamSource {
    void* m_vtbl; // +0x00
    char m_pad04[0x0c - 0x04];
    unsigned long m_0c; // +0x0c  source length
    char m_pad10[0x18 - 0x10];
    unsigned long m_18; // +0x18  read cursor / file position

    int Seek(int pos);                  // 0x139ae0  m_18 = pos
    int Read(void* buf, int n, int at); // 0x139af0  read n bytes (at == -1: current)
};

// The streaming feeder sub-object embedded at StreamVoice+0x6c (a Timer-ish
// pump the trace tagged Timer_1380d0). Its +0x18 word is a cursor that
// FeederReset(0x137dc0) zeroes; FeederStart(0x137d10) arms the pump. The creator
// pre-seeds its data window (+0x2c..+0x3c) before arming.
struct StreamFeeder {
    char m_pad00[0x18];
    unsigned long m_18; // +0x18  cursor
    char m_pad1c[0x2c - 0x1c];
    unsigned long m_2c; // +0x2c  source back-pointer
    unsigned long m_30; // +0x30
    unsigned long m_34; // +0x34
    unsigned long m_38; // +0x38  data length
    unsigned long m_3c; // +0x3c  data back-pointer
    char m_pad40[0x44 - 0x40];

    int FeederStart(SoundStream* owner, void* fmt, int b, int c, StreamVoice* voice, int d);
    // 0x137d10
    void FeederReset(int flag); // 0x137dc0
};

// The per-stream voice object the creator allocates (0xb0 bytes via RezAlloc,
// ctor 0x1375b0). It is a DirectSoundMgr-derived buffer wrapper: +0x0c holds the
// IDirectSoundBuffer to release, +0x6c is the embedded StreamFeeder, and the
// standard +0x04 intrusive link biases +4 (MFC POSITION). On teardown its queued
// voices are reaped, the COM buffer released, the node unlinked, then its
// scalar-deleting destructor runs.
struct StreamVoice {
    void* m_vtbl;                 // +0x00
    StreamVoice* m_link;          // +0x04  next, biased +4 (POSITION)
    void* m_pad08;                // +0x08
    IDirectSoundBufferZ* m_buf0c; // +0x0c  the IDirectSoundBuffer to release
    char m_pad10[0x28 - 0x10];
    unsigned long m_28; // +0x28  duration-ms (= m_2c*1000/m_3c)
    unsigned long m_2c; // +0x2c  byte length
    char m_pad30[0x38 - 0x30];
    unsigned long m_38; // +0x38  avg-bytes-per-sec
    unsigned long m_3c; // +0x3c  avg-bytes-per-sec (divisor)
    char m_pad40[0x6c - 0x40];
    StreamFeeder m_feeder; // +0x6c  embedded streaming feeder sub-object

    // ctor 0x1375b0(IDirectSoundBuffer* buf, SoundStream* owner, int a, int b).
    StreamVoice(IDirectSoundBufferZ* buf, SoundStream* owner, int a, int b);
    void ComputeDuration(); // 0x1359a0  m_28 = m_2c*1000/m_3c
};

// WAVEFORMATEX as the validator reads it: wFormatTag (PCM == 1), then the
// 16-byte PCM tail copied verbatim into the DSBUFFERDESC.lpwfxFormat scratch.
struct WaveFormatX {
    unsigned short wFormatTag;     // +0x00  (== 1: PCM)
    unsigned short nChannels;      // +0x02
    unsigned long nSamplesPerSec;  // +0x04
    unsigned long nAvgBytesPerSec; // +0x08
    unsigned short nBlockAlign;    // +0x0c
    unsigned short wBitsPerSample; // +0x0e
    unsigned short cbSize;         // +0x10
};

class SoundStream : public SoundDevice {
public:
    ~SoundStream(); // 0x137710  restamp vptr (0x5ef6ec) then ~SoundDevice
    StreamVoice* CreateStreamBuffer(WaveFormatX* fmt, unsigned long bytes, int a, int b, int c);
    // 0x137780
    StreamVoice* OpenStream(StreamSource* src, int p1, int p2, int p3, int p4, int p5);
    // 0x137900
    void DestroyVoice(StreamVoice* voice); // 0x1379d0
    int ParseWave(
        StreamSource* src,
        WaveFormatX* fmtBuf,
        unsigned long* outDataOff,
        unsigned long* outDataLen
    ); // 0x137b70
};

#endif // DSNDMGR_SOUNDSTREAM_H
