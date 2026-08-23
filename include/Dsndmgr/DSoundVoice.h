#ifndef DSNDMGR_DSOUNDVOICE_H
#define DSNDMGR_DSOUNDVOICE_H

#include <rva.h>

#include <Dsndmgr/SoundVoiceList.h>

class DirectSoundMgr;

// Tag 1 identifies a volume-ramp voice: DirectSoundMgr::CloneAndPlay cancels the
// buffer's outstanding ramp with RemoveMatching(this, VOICE_TAG_RAMP) before
// constructing the replacement.
struct DSoundVoice : public PureSoundElem {

    virtual i32 Tick(i32 now) OVERRIDE;
    virtual i32 Stop() OVERRIDE;

    i32 m_stopAndRewind;
    i32 m_rampEndVolume;
    i32 m_rampStartVolume;
    i32 m_rampDurationMs;
    i32 m_rampStartTime;

    DSoundVoice(i32 key, i32 pct, i32 mode, DirectSoundMgr* owner, i32 slot, i32 stamp);
};

#endif // DSNDMGR_DSOUNDVOICE_H
