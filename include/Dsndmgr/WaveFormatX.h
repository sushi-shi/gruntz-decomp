// WaveFormatX.h - the WAVEFORMATEX-shaped PCM header the Dsndmgr validators read:
// wFormatTag (PCM == 1), then the 16-byte PCM tail copied verbatim into the
// DSBUFFERDESC.lpwfxFormat scratch. Shared by the device's CreateBuffer/Acquire
// (SoundDevice) and the stream's CreateStreamBuffer/ParseWave (SoundStream) and
// the per-stream voice (StreamVoice).
#ifndef DSNDMGR_WAVEFORMATX_H
#define DSNDMGR_WAVEFORMATX_H

#include <rva.h>

struct WaveFormatX {
    u16 wFormatTag;      // +0x00  (== 1: PCM)
    u16 nChannels;       // +0x02
    u32 nSamplesPerSec;  // +0x04
    u32 nAvgBytesPerSec; // +0x08
    u16 nBlockAlign;     // +0x0c
    u16 wBitsPerSample;  // +0x0e
    u16 cbSize;          // +0x10
};
SIZE_UNKNOWN(WaveFormatX); // WAVEFORMATEX-shaped scratch header

#endif // DSNDMGR_WAVEFORMATX_H
