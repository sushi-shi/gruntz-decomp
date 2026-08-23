#ifndef DSNDMGR_VOLUMESCALE_H
#define DSNDMGR_VOLUMESCALE_H

#include <Enums.h>

// The two volume scales the sound code converts between.
//
// Both directions are written out in GruntzSoundZ, which is what fixes the
// pair - percent to MIDI is `volume * MIDI_VOLUME_MAX / VOLUME_PCT_MAX` with a
// `>= VOLUME_PCT_MAX -> MIDI_VOLUME_MAX` clamp, and MIDI to percent is the
// exact inverse with the clamp the other way round. DirectSoundMgr carries the
// same 100 as a double (c_volScale) for the attenuation curve.
//
// Spelled 100/0x64 and 127/0x7f interchangeably before this header.
//
GZ_ENUM_CONST_BEGIN(VolumeScale)
    VOLUME_PCT_MAX = 100,
    MIDI_VOLUME_MAX = 127
GZ_ENUM_CONST_END(VolumeScale)

inline i32 MidiVolumeToPercent(i32 v) {
    if (v <= 0) {
        return 0;
    }
    if (v >= MIDI_VOLUME_MAX) {
        return VOLUME_PCT_MAX;
    }
    return v * VOLUME_PCT_MAX / MIDI_VOLUME_MAX;
}

// DirectSound's playback-rate limits, as clamped by
// DirectSoundMgr::SetFrequencyPercent. It computes a rate from a percentage
// offset and then pins it strictly INSIDE the range - `>= MAX` becomes MAX - 1
// and `<= MIN` becomes MIN + 1 - which is why the constants are the bounds
// themselves rather than the values assigned.
//
// These are DSBFREQUENCY_MIN and DSBFREQUENCY_MAX in the DirectX SDK, but the
// dsound.h that MSVC 5.0 ships predates those macros, so they are spelled out
// here rather than included.
GZ_ENUM_CONST_BEGIN(DSoundFrequency)
    DSOUND_FREQUENCY_MIN = 100,
    DSOUND_FREQUENCY_MAX = 100000
GZ_ENUM_CONST_END(DSoundFrequency)

#endif // DSNDMGR_VOLUMESCALE_H
