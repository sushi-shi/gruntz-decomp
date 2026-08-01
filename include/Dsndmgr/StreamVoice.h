#ifndef DSNDMGR_STREAMVOICE_H
#define DSNDMGR_STREAMVOICE_H

#include <Gruntz/ParseSource.h>
#include <rva.h>

#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/StreamFeeder.h>
#include <Dsndmgr/WaveFormatX.h>

struct IDirectSoundBuffer;

class SoundStream;

struct StreamVoice : public DSoundCloneInst {

    StreamVoice(IDirectSoundBuffer* buf, SoundStream* owner, i32 a, i32 b);

    virtual ~StreamVoice() OVERRIDE;

    i32 SetSource(CParseSource* src);
    i32 Configure(i32 vol, i32 pan, i32 freq, i32 loop);
    u32 ComputeRatio();

    i32 m_stopWhenIdle;
    i32 m_retireWhenIdle;
    i32 m_active;

    StreamVoiceFeeder m_feeder;
};
SIZE(0xb0);

#endif // DSNDMGR_STREAMVOICE_H
