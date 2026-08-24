#ifndef DSNDMGR_STREAMVOICE_H
#define DSNDMGR_STREAMVOICE_H

#include <rva.h>

#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/StreamFeeder.h>
#include <Dsndmgr/WaveFormatX.h>
#include <Gruntz/ParseSource.h>

struct IDirectSoundBuffer;

class SoundStream;

struct StreamVoice : public SoundSample {

    StreamVoice(
        IDirectSoundBuffer* buffer,
        SoundStream* owner,
        i32 reprimeWhenIdle,
        i32 destroyWhenIdle
    );

    virtual ~StreamVoice() OVERRIDE;

    i32 SetSource(CParseSource* source);
    i32 Configure(i32 volumePct, i32 panPct, i32 frequencyOffsetPct, i32 looping);
    u32 GetDurationMs();

    i32 m_reprimeWhenIdle;
    i32 m_destroyWhenIdle;
    i32 m_wasPlaying;

    StreamVoiceFeeder m_feeder;
};

#endif // DSNDMGR_STREAMVOICE_H
