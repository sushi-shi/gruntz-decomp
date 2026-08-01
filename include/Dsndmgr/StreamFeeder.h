#ifndef DSNDMGR_STREAMFEEDER_H
#define DSNDMGR_STREAMFEEDER_H

#include <rva.h>

#include <Dsndmgr/WaveFormatX.h>
#include <Gruntz/ParseSource.h>

class SoundDevice;

class DirectSoundMgr;

struct StreamFeeder {
    virtual i32 Feed(void* dst1, u32 n1, u32* got1, void* dst2, u32 n2, u32* got2) = 0;

    virtual i32 FeedData();
    virtual void OnDrain();

    SoundDevice* m_owner;
    DirectSoundMgr* m_buffer;
    u32 m_bufferCursor;
    u32 m_bufferLength;
    u32 m_format;
    u32 m_armed;
    u32 m_drained;
    u32 m_pendingBytes;
    u8 m_silenceByte;
    u32 m_lastTickMs;
    CParseSource* m_source;
    u32 m_loop;
    u32 m_sourceOffset;
    u32 m_windowStart;
    u32 m_windowLength;
    u32 m_windowEnd;

    i32 SeedWindow(CParseSource* src, u32 off, u32 len);
    StreamFeeder();

    ~StreamFeeder();
    i32 FeederStart(
        SoundDevice* owner,
        WaveFormatX* fmt,
        u32 len,
        u32 format,
        DirectSoundMgr* buf,
        i32 tickArg
    );
    void FeederReset(i32 doStop);
    i32 Resume();
    i32 Pause();
    i32 FillBuffer(u32 writePos, u32 bytes);

    i32 Tick(i32 timestamp);

    i32 TickPump(i32 now);
};
SIZE(0x44);

struct StreamVoiceFeeder : StreamFeeder {
    StreamVoiceFeeder() {}
    virtual i32 Feed(void* dst1, u32 n1, u32* got1, void* dst2, u32 n2, u32* got2) OVERRIDE;
    virtual i32 FeedData() OVERRIDE;
    virtual void OnDrain() OVERRIDE;
};
SIZE(0x44);

#endif // DSNDMGR_STREAMFEEDER_H
