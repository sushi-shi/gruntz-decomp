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
// RETAIL OVERRUN, recorded not fixed. g_volumeTable is 100 entries - the span
// from its DATA pin at 0x253ab8 to g_panTable at 0x253c48 is exactly 400 bytes -
// but SoundDevice::BuildVolumeTable fills it with `i <= VOLUME_PCT_MAX`, which
// is 101 stores. The binary settles it beyond argument: retail's loop is
// `mov esi, 0x653ab8` ... `cmp esi, 0x653c48; jle`, so the terminating compare
// is against g_panTable's own address and the last store lands ON it. Our
// reconstruction is byte-identical. The bound keeps its `<=` spelling so the
// defect stays visible; a percent scale of 0..100 needs 101 slots, and the
// table has 100.
GZ_ENUM_CONST_BEGIN(VolumeScale)
    VOLUME_PCT_MAX = 100,
    MIDI_VOLUME_MAX = 127
GZ_ENUM_CONST_END(VolumeScale)

#endif // DSNDMGR_VOLUMESCALE_H
