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
SIZE(WaveFormatX, 0x14); // WAVEFORMATEX-shaped PCM header (u16 tail padded to a 4-byte multiple)

#endif // DSNDMGR_WAVEFORMATX_H
