#ifndef DSNDMGR_WAVEFORMATSDK_H
#define DSNDMGR_WAVEFORMATSDK_H

#include <Mfc.h>

#include <Dsndmgr/WaveFormatX.h>

#include <mmsystem.h>

// WaveFormatX is the retail custom signature type; DirectSound's ABI names the
// layout WAVEFORMATEX. Both are the same proven packed 18-byte record.
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
