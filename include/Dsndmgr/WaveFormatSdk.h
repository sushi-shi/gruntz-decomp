#ifndef DSNDMGR_WAVEFORMATSDK_H
#define DSNDMGR_WAVEFORMATSDK_H

#include <Mfc.h>

#include <Dsndmgr/WaveFormatX.h>

#include <mmsystem.h>

union WaveFormatSdkPtr {
    WaveFormatX* m_format;
    LPWAVEFORMATEX m_sdk;
};

inline LPWAVEFORMATEX WaveFormatSdk(WaveFormatX* format) {
    WaveFormatSdkPtr value;
    value.m_format = format;
    return value.m_sdk;
}

#endif // DSNDMGR_WAVEFORMATSDK_H
