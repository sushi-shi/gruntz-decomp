#ifndef DSNDMGR_WAVEFORMATPTR_H
#define DSNDMGR_WAVEFORMATPTR_H

#include <Dsndmgr/WaveFormatX.h>
#include <mmsystem.h>

union WaveFormatPtr {
    WaveFormatX* m_rec;
    LPWAVEFORMATEX m_sdk;
};

#endif // DSNDMGR_WAVEFORMATPTR_H
