#ifndef GRUNTZ_SOUNDCUEREGISTRYINLINE_H
#define GRUNTZ_SOUNDCUEREGISTRYINLINE_H

#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/SoundStream.h>
#include <Gruntz/SoundCueInline.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Rez/FrameClock.h>

inline void SoundCueRegistry::TickVolumeRamps() {
    if (m_soundStream != NULL) {
        m_soundStream->TickVolumeRamps(-1);
    }
}

static __inline i32 PlayRegistryCueIfElapsed(SoundCueRegistry* soundRegistry, const char* cueKey) {
    if (!soundRegistry->m_silentMode) {
        SoundCue* cue = soundRegistry->FindCue(cueKey);
        if (cue != NULL) {
            return PlaySoundCueIfElapsed(cue, g_soundVolumePercent, 0, 0, false);
        }
    }
    return 0;
}

#endif // GRUNTZ_SOUNDCUEREGISTRYINLINE_H
