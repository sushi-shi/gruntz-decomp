#ifndef GRUNTZ_AMBIENTSOUNDINLINE_H
#define GRUNTZ_AMBIENTSOUNDINLINE_H

#include <Gruntz/AmbientSound.h>

inline i32 ScaleAmbientVolume(CAmbientSound* sound, i32 level) {
    i32 scale = sound->m_scaleA;
    if (scale > 5) {
        scale -= 0xf;
    }
    i32 volume = (scale * level) / 100;
    if (sound->m_scaleB > 0) {
        volume = (volume * sound->m_scaleB) / 100;
    }
    if (volume < 0) {
        return 0;
    }
    if (volume > 0x64) {
        return 0x64;
    }
    return volume;
}

#endif // GRUNTZ_AMBIENTSOUNDINLINE_H
