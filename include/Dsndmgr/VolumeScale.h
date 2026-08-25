#ifndef DSNDMGR_VOLUMESCALE_H
#define DSNDMGR_VOLUMESCALE_H

#include <Enums.h>

GZ_ENUM_CONST_BEGIN(VolumeScale)
    VOLUME_PCT_MAX = 100,
    MIDI_VOLUME_MAX = 127
GZ_ENUM_CONST_END(VolumeScale)

inline i32 MidiVolumeToPercent(i32 midiVolume) {
    if (midiVolume <= 0) {
        return 0;
    }
    if (midiVolume >= MIDI_VOLUME_MAX) {
        return VOLUME_PCT_MAX;
    }
    return midiVolume * VOLUME_PCT_MAX / MIDI_VOLUME_MAX;
}

GZ_ENUM_CONST_BEGIN(DSoundFrequency)
    DSOUND_FREQUENCY_MIN = 100,
    DSOUND_FREQUENCY_MAX = 100000
GZ_ENUM_CONST_END(DSoundFrequency)

#endif // DSNDMGR_VOLUMESCALE_H
