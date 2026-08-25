#ifndef GRUNTZ_SOUNDCUEREGISTRYINLINE_H
#define GRUNTZ_SOUNDCUEREGISTRYINLINE_H

#include <Dsndmgr/SoundStream.h>
#include <Gruntz/SoundCueRegistry.h>

inline void TickSoundVolumeRamps(SoundCueRegistry* registry) {
    if (registry->m_soundStream != NULL) {
        registry->m_soundStream->TickVolumeRamps(-1);
    }
}

#endif // GRUNTZ_SOUNDCUEREGISTRYINLINE_H
