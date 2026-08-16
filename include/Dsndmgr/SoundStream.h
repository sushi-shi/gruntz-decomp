#ifndef DSNDMGR_SOUNDSTREAM_H
#define DSNDMGR_SOUNDSTREAM_H

#include <rva.h>

#include <Dsndmgr/SoundDevice.h>
#include <Dsndmgr/SoundVoiceList.h>
#include <Dsndmgr/StreamVoice.h>
#include <Gruntz/ParseSource.h>

class SoundStream;

class SoundStream : public SoundDevice {
public:
    SoundStream();
    virtual ~SoundStream() OVERRIDE;

    StreamVoice* CreateStreamBuffer(
        WaveFormatX* fmt,
        u32 bytes,
        i32 dsFlags,
        i32 stopWhenIdle,
        i32 retireWhenIdle
    );

    StreamVoice* OpenStream(
        CParseSource* src,
        i32 bytes,
        i32 format,
        i32 dsFlags,
        i32 stopWhenIdle,
        i32 retireWhenIdle
    );
    StreamVoice* PlayStream(CParseSource* src, i32 bytes, i32 format, i32 dsFlags);

    void DestroyVoice(StreamVoice* voice);

    void Free();

    void Stop();

    i32 PlaySoundDefaulted(void* hWnd, i32 flag);

    i32 TickSubManagers(i32 time);
    i32 ParseWave(CParseSource* src, WaveFormatX* fmtBuf, u32* outDataOff, u32* outDataLen);

    DSoundList m_voices;
};

extern i32 g_ssLogEnabled;
extern i32 g_ssMsgBoxEnabled;
extern i32 g_ssBeepEnabled;
extern i32 g_ssThirdEnabled;

#endif // DSNDMGR_SOUNDSTREAM_H
