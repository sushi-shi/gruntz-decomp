#ifndef DSNDMGR_RIFFWAVE_H
#define DSNDMGR_RIFFWAVE_H

#include <Mfc.h>

#include <Ints.h>

#include <mmsystem.h>

#pragma pack(push, 1)
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

#endif // DSNDMGR_RIFFWAVE_H
