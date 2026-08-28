#ifndef DSNDMGR_SOUNDSTREAM_H
#define DSNDMGR_SOUNDSTREAM_H

#include <rva.h>

#include <Dsndmgr/SoundDevice.h>
#include <Dsndmgr/StreamVoice.h>
#include <Lith/BaseList.h>
#include <Rez/RezArchiveEntry.h>

class SoundStream;

class SoundStream : public SoundDevice {
public:
    SoundStream();
    virtual ~SoundStream() OVERRIDE;

    StreamVoice* CreateStreamVoice(
        WaveFormatX* format,
        u32 bufferBytes,
        i32 dsFlags,
        i32 reprimeWhenIdle,
        i32 destroyWhenIdle
    );

    StreamVoice* OpenStream(
        CRezArchiveEntry* src,
        i32 bufferBytes,
        i32 refillThresholdBytes,
        i32 dsFlags,
        i32 reprimeWhenIdle,
        i32 destroyWhenIdle
    );
    StreamVoice*
    PlayStream(CRezArchiveEntry* source, i32 bufferBytes, i32 refillThresholdBytes, i32 dsFlags);

    void DestroyVoice(StreamVoice* voice);

    void ShutdownStreams();

    void StopAllStreams();

    i32 InitializeDevice(HWND hwnd, i32 cooperativeLevel);

    i32 TickStreams(i32 timestampMs);
    i32 ParseWave(
        CRezArchiveEntry* source,
        WaveFormatX* outFormat,
        u32* outDataOffset,
        u32* outDataBytes
    );

    CLTBaseList m_voices;
};

extern b32 g_dsoundDebugLog;
extern b32 g_dsoundErrorDialogs;
extern b32 g_dsoundErrorBeeps;
extern b32 g_dsoundFormatErrors;

#endif // DSNDMGR_SOUNDSTREAM_H
