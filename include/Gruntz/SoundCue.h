#ifndef GRUNTZ_SOUNDCUE_H
#define GRUNTZ_SOUNDCUE_H

#include <Ints.h>

#include <Ints.h>
#include <Utils/MapTyped.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class SoundSample;
struct CRezItm;
struct RiffWaveHeader;

struct SoundCue : public CWapObj {

    virtual i32 IsLoaded()   {
        return m_sound != NULL;
    }

    virtual void Unload()  ;

    virtual ~SoundCue()  ;

    SoundCue(i32 cueId, class CDDrawSurfaceMgr* owner);

    i32 LoadFromWave(RiffWaveHeader* riff);
    i32 LoadFromFile(char* path);
    i32 LoadFromSource(CRezItm* source);

    i32 PlayIfElapsed(i32 volumePercent, i32 panPercent, i32 frequencyOffsetPercent, b32 looping);

    i32 PlaySpatialized(i32 sourceX, i32 listenerX, i32 maxPanOffsetPx, i32 fullPanOffsetPx);

    SoundSample* m_sound;
    i32 m_lastPlayTimeMs;
    i32 m_replayDelayMs;
};
inline SoundCue::SoundCue(i32 cueId, CDDrawSurfaceMgr* owner)
    : CWapObj(owner, cueId, 0, CWapObj::NO_SEED) {
    m_sound = NULL;
    m_replayDelayMs = 0;
    m_lastPlayTimeMs = 0;
}

#endif
