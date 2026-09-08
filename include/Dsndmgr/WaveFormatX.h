#ifndef DSNDMGR_WAVEFORMATX_H
#define DSNDMGR_WAVEFORMATX_H

#include <rva.h>

#pragma pack(push, 1)
struct WaveFormatX {
    u16 m_wFormatTag;
    u16 m_nChannels;
    u32 m_nSamplesPerSec;
    u32 m_nAvgBytesPerSec;
    u16 m_nBlockAlign;
    u16 m_wBitsPerSample;
    u16 m_cbSize;
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
