// WaveFormatX.h - Monolith's OWN WAVEFORMATEX-shaped PCM header the Dsndmgr
// validators read: wFormatTag (PCM == 1), then the 16-byte PCM tail copied
// verbatim into the DSBUFFERDESC.lpwfxFormat scratch. Shared by the device's
// CreateBuffer/Acquire (SoundDevice), the stream's CreateStreamBuffer/ParseWave
// (SoundStream) and the per-stream voice (StreamVoice).
//
// PERMANENT-KEEP - this is NOT scaffolding for the SDK's <mmsystem.h> WAVEFORMATEX,
// and it must NOT be migrated to it (docs/comdefs-removal-plan.md step 3 =
// CANCELLED-FOR-CAUSE). The SDK WAVEFORMATEX is 0x12 bytes - `#pragma pack(1)` in
// EVERY SDK era (pshpack1.h) - but the game's struct is 0x14: it is UNPACKED, so the
// trailing `u16 cbSize` pads to a 4-byte multiple. The DirectSound TUs byte-match at
// 0x14, so WaveFormatX is the faithful reconstruction of the devs' own struct;
// swapping to the packed 0x12 WAVEFORMATEX would shrink every containing layout and
// break matches. Probe: static_assert(sizeof(WAVEFORMATEX)==0x12) and
// static_assert(sizeof(WaveFormatX)==0x14) both pass (wine-cl + clang).
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
