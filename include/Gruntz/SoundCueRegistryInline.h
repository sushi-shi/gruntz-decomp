#ifndef GRUNTZ_SOUNDCUEREGISTRYINLINE_H
#define GRUNTZ_SOUNDCUEREGISTRYINLINE_H

#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/SoundStream.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Rez/FrameClock.h>

inline void TickSoundVolumeRamps(SoundCueRegistry* registry) {
    if (registry->m_soundStream != NULL) {
        registry->m_soundStream->TickVolumeRamps(-1);
    }
}

static __inline i32 PlayMenuCue(SoundCueRegistry* soundRegistry, const char* cueKey) {
    if (!soundRegistry->m_silentMode) {
        SoundCue* foundCue = soundRegistry->FindCue(cueKey);
        SoundCue* cue = foundCue;
        if (cue != NULL) {
            b32 soundEnabled = g_soundEnabled;
            i32 volumePercent = g_soundVolumePercent;
            if (soundEnabled != false) {
                i32 cueTimeMs = g_soundCueTimeMs;
                u32 elapsedMs =
                    static_cast<u32>(cueTimeMs) - static_cast<u32>(cue->m_lastPlayTimeMs);
                if (elapsedMs >= static_cast<u32>(cue->m_replayDelayMs)) {
                    cue->m_lastPlayTimeMs = cueTimeMs;
                    return cue->m_sound->AcquireAndPlay(volumePercent, 0, 0, false);
                }
            }
        }
    }
    return 0;
}

#endif // GRUNTZ_SOUNDCUEREGISTRYINLINE_H
