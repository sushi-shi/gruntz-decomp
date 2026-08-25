#ifndef GRUNTZ_GRUNTZ_LEAFCUE_H
#define GRUNTZ_GRUNTZ_LEAFCUE_H

#include <rva.h>

#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class SoundSample;
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

    i32 PlayIfElapsed(i32 volumePercent, i32 panPercent, i32 frequencyOffsetPercent, i32 looping);

    i32 PlaySpatialized(i32 sourceX, i32 listenerX, i32 maxPanOffsetPx, i32 fullPanOffsetPx);

    SoundSample* m_sound;
    i32 m_lastPlayTimeMs;
    i32 m_replayDelayMs;
};
inline LeafCue::LeafCue(i32 count, CDDrawSurfaceMgr* handle)
    : CWapObj(handle, count, 0, CWapObj::NO_SEED) {
    m_sound = NULL;
    m_replayDelayMs = 0;
    m_lastPlayTimeMs = 0;
}

#endif // GRUNTZ_GRUNTZ_LEAFCUE_H
