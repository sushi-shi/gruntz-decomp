#ifndef INCLUDE_GRUNTZ_SOUNDSTATE_H
#define INCLUDE_GRUNTZ_SOUNDSTATE_H
#include <Ints.h>

// The one PROVEN value of the cue tag. CDDrawSubMgrLeafScan::BindSoundStream
// (0x157a80) resets g_sndCueTag to 100 whenever the sound stream is (re)bound, and
// the pan/volume mixer (0x1587a0) special-cases exactly that value as "apply no
// scaling" (`if (cue == 100) vscale = amp;`). So 100 is the neutral/unattenuated
// tag. No other tag value is proven anywhere, so none is enumerated.
typedef enum SoundCueTag {
    SND_CUE_NEUTRAL = 100, // no cue scaling (the bind-time default)
} SoundCueTag;

extern i32 g_sndEnabled; // 0x61ab20  sound-on gate
extern i32 g_sndCueTag;  // 0x61ab24  current cue-item id

#endif // INCLUDE_GRUNTZ_SOUNDSTATE_H
