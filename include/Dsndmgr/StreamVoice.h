#ifndef DSNDMGR_STREAMVOICE_H
#define DSNDMGR_STREAMVOICE_H

#include <rva.h>

#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/StreamFeeder.h>
#include <Dsndmgr/WaveFormatX.h>
#include <Rez/RezArchiveEntry.h>

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

    i32 SetSource(CRezItm* source);
    i32 Configure(i32 volumePct, i32 panPct, i32 frequencyOffsetPct, b32 looping);
    u32 GetDurationMs();

    b32 m_reprimeWhenIdle;
    b32 m_destroyWhenIdle;
    b32 m_wasPlaying;

    StreamVoiceFeeder m_feeder;
};

#endif // DSNDMGR_STREAMVOICE_H
