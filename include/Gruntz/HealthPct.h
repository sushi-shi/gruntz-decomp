#ifndef GRUNTZ_GRUNTZ_HEALTHPCT_H
#define GRUNTZ_GRUNTZ_HEALTHPCT_H

#include <Enums.h>

// CGrunt::m_health is a PERCENTAGE, 0 to 100.
//
// The three health powerups prove the top between them: each adds its bute
// amount (`GetIntDef("Powerupz", "Health1", 0x19)` and its siblings) and then
// clamps with the identical two lines, `if (h >= HEALTH_FULL) h = HEALTH_FULL`.
// A clamp that three independent arms share is a scale bound, not a threshold.
//
// The CUE_HEAL combat cue and the fresh-grunt paths set it straight to
// HEALTH_FULL, and death is tested as `m_health <= HEALTH_EMPTY` - so both ends
// of the scale are exercised.
GZ_ENUM_CONST_BEGIN(HealthPct)
    HEALTH_EMPTY = 0,
    HEALTH_FULL = 100
GZ_ENUM_CONST_END(HealthPct)

#endif // GRUNTZ_GRUNTZ_HEALTHPCT_H
