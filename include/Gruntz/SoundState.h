// SoundState.h - the two global sound-cue state ints.
//
// g_sndEnabled (?g_sndEnabled@@3HA @0x61ab20, retail .data init 1) is the sound-on gate
// (mirrors CGruntzMgr::m_soundEnabled); g_sndCueTag (?g_sndCueTag@@3HA @0x61ab24, init 100)
// is the current cue-item id the LeafCue::PlayIfElapsed paths key off. Both are DEFINED +
// DATA-pinned in src/Gruntz/GruntzMgr.cpp; C++-linkage ints, so a reference is DATA-reloc-
// masked. Declared once here (a minimal owner header, distinct from the CSndHost subsystem
// header <Gruntz/SoundCue.h>) so consumers stop re-`extern`-ing them per-TU and the headers
// that used to (Globals.h/InGameIcon.h) share it.
#ifndef INCLUDE_GRUNTZ_SOUNDSTATE_H
#define INCLUDE_GRUNTZ_SOUNDSTATE_H
#include <Ints.h>

extern i32 g_sndEnabled; // 0x61ab20  sound-on gate
extern i32 g_sndCueTag;  // 0x61ab24  current cue-item id

#endif // INCLUDE_GRUNTZ_SOUNDSTATE_H
