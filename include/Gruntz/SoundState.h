#ifndef INCLUDE_GRUNTZ_SOUNDSTATE_H
#define INCLUDE_GRUNTZ_SOUNDSTATE_H

#include <Enums.h>
#include <Ints.h>

// MIS-NAMED, do not apply: this is the sound VOLUME, not a cue tag.
// `CGruntzMgr` writes it from the volume slider (`m_soundVolume = v;
// g_sndCueTag = v;`) and 100 is full scale (`abs(SND_CUE_NEUTRAL)` is used
// as a percentage). Rename both the global and this domain before typing.
GZ_ENUM_BEGIN(SoundCueTag)
    SND_CUE_NEUTRAL = 100
GZ_ENUM_END(SoundCueTag)

extern i32 g_sndEnabled;
extern i32 g_sndCueTag;

#endif // INCLUDE_GRUNTZ_SOUNDSTATE_H
