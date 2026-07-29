#ifndef DSNDMGR_WAVEFORMATX_H
#define DSNDMGR_WAVEFORMATX_H

#include <rva.h>

// Retail reads and copies the two u16 PAIRS as single dwords (SoundDevice::CreateBuffer
// @0x1366f0 moves the header as dword@0 / dword@4 / dword@8 / dword@0xc / word@0x10, and
// @0x136808 loads dword@0 straight into DSoundCloneInst::m_freq). Both readings of those
// four bytes are real, so they are modelled as unions - the documented device for a
// proven two-readings-of-one-field, same as BrickzCell's flag dword. Naming them here
// retires three reinterpret_casts in DirectSoundMgr.cpp; the layout is unchanged.
struct WaveFormatX {
    union {
        u32 m_formatWord; // +0x00  wFormatTag | nChannels<<16, as retail moves it
        struct {
            u16 wFormatTag; // +0x00  (== 1: PCM)
            u16 nChannels;  // +0x02
        };
    };
    u32 nSamplesPerSec;  // +0x04
    u32 nAvgBytesPerSec; // +0x08
    union {
        u32 m_blockWord; // +0x0c  nBlockAlign | wBitsPerSample<<16
        struct {
            u16 nBlockAlign;    // +0x0c
            u16 wBitsPerSample; // +0x0e
        };
    };
    u16 cbSize; // +0x10
};
SIZE(0x14); // WAVEFORMATEX-shaped PCM header (u16 tail padded to a 4-byte multiple)

// The RIFF chunk cursor. A RIFF walk reads DWORD ids/sizes but advances by a
// BYTE count padded to even, and the 'fmt ' chunk body IS a WaveFormatX at the
// cursor - three readings of one pointer, named here so the walk has no pun.
union RiffCursor {
    u32* m_w;           // the dword reads (id, size)
    char* m_b;          // the byte advance / end compare
    WaveFormatX* m_fmt; // the 'fmt ' chunk body at the cursor
};

#endif // DSNDMGR_WAVEFORMATX_H
