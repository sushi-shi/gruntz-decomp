#ifndef GRUNTZ_SOUNDCUEINLINE_H
#define GRUNTZ_SOUNDCUEINLINE_H

#include <Dsndmgr/SoundBuffer.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Rez/FrameClock.h>

inline i32 PlaySoundCueIfElapsed(
    SoundCue* cue,
    i32 volumePercent,
    i32 panPercent,
    i32 frequencyOffsetPercent,
    i32 looping
) {
    if (g_soundEnabled == 0) {
        return 0;
    }
    if (g_soundCueTimeMs - static_cast<u32>(cue->m_lastPlayTimeMs)
        < static_cast<u32>(cue->m_replayDelayMs)) {
        return 0;
    }
    cue->m_lastPlayTimeMs = g_soundCueTimeMs;
    return cue->m_sound->AcquireAndPlay(volumePercent, panPercent, frequencyOffsetPercent, looping);
}

#endif // GRUNTZ_SOUNDCUEINLINE_H
