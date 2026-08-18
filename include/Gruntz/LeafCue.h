#ifndef GRUNTZ_GRUNTZ_LEAFCUE_H
#define GRUNTZ_GRUNTZ_LEAFCUE_H

#include <rva.h>

#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class DSoundCloneInst;
struct CParseSource;
struct RiffWaveHeader;

struct LeafCue : public CWapObj {

    RVA(0x00158650, 0xb)
    virtual i32 IsLoaded() OVERRIDE {
        return m_sound != NULL;
    }

    virtual void Unload() OVERRIDE;

    virtual ~LeafCue() OVERRIDE;

    LeafCue(i32 count, class CDDrawSurfaceMgr* handle);

    i32 LoadSoundA(RiffWaveHeader* riff);
    i32 LoadSoundB(char* src);
    i32 Configure(CParseSource* src);

    i32 PlayIfElapsed(i32 vol, i32 pan, i32 freqPct, i32 loop);

    i32 TriggerBlit(i32 pos, i32 center, i32 range1, i32 range2);

    DSoundCloneInst* m_sound;
    i32 m_lastPlayTime;
    i32 m_replayDelay;
};
inline LeafCue::LeafCue(i32 count, CDDrawSurfaceMgr* handle)
    : CWapObj(handle, count, 0, CWapObj::NO_SEED) {
    m_sound = NULL;
    m_replayDelay = 0;
    m_lastPlayTime = 0;
}

#endif // GRUNTZ_GRUNTZ_LEAFCUE_H
