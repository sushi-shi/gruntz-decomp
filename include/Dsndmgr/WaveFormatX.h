#ifndef DSNDMGR_WAVEFORMATX_H
#define DSNDMGR_WAVEFORMATX_H

#include <rva.h>

// 18 bytes, exactly the SDK's WAVEFORMATEX (mmsystem.h declares its structs under
// pshpack1). PROVEN by retail's copy shape: SoundDevice::CreateBuffer and
// SoundStream::CreateStreamBuffer both copy `*fmt` as FOUR dwords plus ONE word -
// cl only unrolls a struct assignment that way when the size is not a multiple of
// 4. A 0x14-byte spelling makes cl emit `mov ecx,5; rep movsd` instead, which also
// burns esi/edi and reshapes the whole function's register allocation.
#pragma pack(push, 1)
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
#pragma pack(pop)
SIZE(0x12);

union RiffCursor {
    u32* m_words;
    char* m_bytes;
    WaveFormatX* m_format;
};

#endif // DSNDMGR_WAVEFORMATX_H
