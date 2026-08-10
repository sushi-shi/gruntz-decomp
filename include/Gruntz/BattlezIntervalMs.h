#ifndef GRUNTZ_GRUNTZ_BATTLEZINTERVALMS_H
#define GRUNTZ_GRUNTZ_BATTLEZINTERVALMS_H

#include <Enums.h>

// Millisecond windows the Battlez map driver arms against g_frameTime.
//
// CBattlezMapConfig::RouteToNearbyEnemy fires voice cue 0x366 ("can't get
// there") only when the unit's route-blocked window has expired, then re-arms
// it: it zeroes both halves of the clock pair at +0x78 and the window pair at
// +0x80, writes 0x1388 into the window and stamps g_frameTime into the clock.
GZ_ENUM_CONST_BEGIN(BattlezIntervalMs)
    BLOCKED_VOICE_INTERVAL_MS = 0x1388
GZ_ENUM_CONST_END(BattlezIntervalMs)

#endif // GRUNTZ_GRUNTZ_BATTLEZINTERVALMS_H
