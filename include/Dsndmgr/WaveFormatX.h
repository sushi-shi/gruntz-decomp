#ifndef DSNDMGR_WAVEFORMATX_H
#define DSNDMGR_WAVEFORMATX_H

#include <rva.h>

// 18 bytes, exactly the SDK's WAVEFORMATEX (mmsystem.h declares its structs under
// pshpack1). PROVEN by retail's copy shape: SoundDevice::CreateSample and
// SoundStream::CreateStreamVoice both copy `*fmt` as FOUR dwords plus ONE word -
// cl only unrolls a struct assignment that way when the size is not a multiple of
// 4. A 0x14-byte spelling makes cl emit `mov ecx,5; rep movsd` instead, which also
// burns esi/edi and reshapes the whole function's register allocation.
#pragma pack(push, 1)
struct WaveFormatX {
    u16 wFormatTag;
    u16 nChannels;
    u32 nSamplesPerSec;
    u32 nAvgBytesPerSec;
    u16 nBlockAlign;
    u16 wBitsPerSample;
    u16 cbSize;
};

struct RiffWaveHeader {
    u32 m_riffTag;
    u32 m_riffSize;
    u32 m_waveTag;
    u8 m_chunks[1];
};

struct RiffChunkHeader {
    u32 m_id;
    u32 m_size;
    u8 m_data[1];
};
#pragma pack(pop)

#endif // DSNDMGR_WAVEFORMATX_H
