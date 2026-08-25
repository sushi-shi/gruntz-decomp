#ifndef DSNDMGR_WAVEFORMATX_H
#define DSNDMGR_WAVEFORMATX_H

#include <rva.h>

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
