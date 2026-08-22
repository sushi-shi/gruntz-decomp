#ifndef GRUNTZ_LEAFCUEINLINE_H
#define GRUNTZ_LEAFCUEINLINE_H

#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/SoundState.h>
#include <Rez/FrameClock.h>

inline i32 PlayLeafCueIfElapsed(LeafCue* cue, i32 vol, i32 pan, i32 freqPct, i32 loop) {
    if (g_sndEnabled == 0) {
        return 0;
    }
    if (g_killCueClock - static_cast<u32>(cue->m_lastPlayTime)
        < static_cast<u32>(cue->m_replayDelay)) {
        return 0;
    }
    cue->m_lastPlayTime = g_killCueClock;
    return cue->m_sound->ConfigureItem(vol, pan, freqPct, loop);
}

#endif // GRUNTZ_LEAFCUEINLINE_H
