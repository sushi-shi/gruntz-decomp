#ifndef INCLUDE_GRUNTZ_SOUNDSTATE_H
#define INCLUDE_GRUNTZ_SOUNDSTATE_H
#include <Ints.h>

typedef enum SoundCueTag {
    SND_CUE_NEUTRAL = 100,
} SoundCueTag;

extern i32 g_sndEnabled;
extern i32 g_sndCueTag;

#endif // INCLUDE_GRUNTZ_SOUNDSTATE_H
