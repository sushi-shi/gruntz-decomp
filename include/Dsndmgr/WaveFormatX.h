#ifndef DSNDMGR_WAVEFORMATX_H
#define DSNDMGR_WAVEFORMATX_H

#include <rva.h>

struct WaveFormatX {
    union {
        u32 m_formatWord;
        struct {
            u16 wFormatTag;
            u16 nChannels;
        };
    };
    u32 nSamplesPerSec;
    u32 nAvgBytesPerSec;
    union {
        u32 m_blockWord;
        struct {
            u16 nBlockAlign;
            u16 wBitsPerSample;
        };
    };
    u16 cbSize;
};
SIZE(0x14);

union RiffCursor {
    u32* m_w;
    char* m_b;
    WaveFormatX* m_fmt;
};

#endif // DSNDMGR_WAVEFORMATX_H
