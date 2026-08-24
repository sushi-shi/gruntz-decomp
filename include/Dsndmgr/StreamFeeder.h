#ifndef DSNDMGR_STREAMFEEDER_H
#define DSNDMGR_STREAMFEEDER_H

#include <rva.h>

#include <Dsndmgr/WaveFormatX.h>
#include <Gruntz/ParseSource.h>

class SoundDevice;

class SoundBuffer;

struct StreamFeeder {
    virtual i32 Feed(u8* dst1, u32 bytes1, u32* filled1, u8* dst2, u32 bytes2, u32* filled2) = 0;

    virtual i32 ResetSource();
    virtual void OnReset();

    SoundDevice* m_owner;
    SoundBuffer* m_buffer;
    u32 m_writeCursor;
    u32 m_bufferBytes;
    u32 m_refillThresholdBytes;
    u32 m_initialized;
    u32 m_playing;
    u32 m_silenceBytes;
    u8 m_silenceByte;
    u32 m_lastTickMs;
    CParseSource* m_source;
    u32 m_looping;
    u32 m_sourceOffset;
    u32 m_windowStart;
    u32 m_windowLength;
    u32 m_windowEnd;

    i32 SeedWindow(CParseSource* source, u32 offset, u32 bytes);
    StreamFeeder();

    ~StreamFeeder();
    i32 Initialize(
        SoundDevice* owner,
        WaveFormatX* format,
        u32 bufferBytes,
        u32 refillThresholdBytes,
        SoundBuffer* buffer,
        i32 initialTickMs
    );
    void Reset(i32 destroyBuffer);
    i32 Resume();
    i32 Pause();
    i32 FillBuffer(u32 writePos, u32 bytes);

    i32 Tick(i32 timestampMs);

    i32 PrimeBuffer(i32 timestampMs);
};

struct StreamVoiceFeeder : StreamFeeder {
    StreamVoiceFeeder() {}
    virtual i32 Feed(u8* dst1, u32 bytes1, u32* filled1, u8* dst2, u32 bytes2, u32* filled2)
        OVERRIDE;
    virtual i32 ResetSource() OVERRIDE;
    virtual void OnReset() OVERRIDE;
};

#endif // DSNDMGR_STREAMFEEDER_H
