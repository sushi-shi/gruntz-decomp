#ifndef GRUNTZ_GRUNTZ_SBIHLROWSTATE_H
#define GRUNTZ_GRUNTZ_SBIHLROWSTATE_H

#include <Enums.h>

// The animation state of a status-bar highlight row (CSbiHlRow). m_counter is
// the frame index the row is showing, and every state is named by the direction
// it drives that counter and the target it drives it to - all read off the arms,
// none inferred:
//
//   IDLE_CYCLE   ++ and wraps back to 1 past 9      the resting loop, frames 1..9
//   RAMP_UP_LOW  ++ until 0x12, then -> HOLD_LOW
//   HOLD_LOW     dwells on a cue clock, then -> RAMP_DOWN_LOW
//   RAMP_DOWN_LOW  -- until below 0xa, then -> OFF with the counter reset to 1
//   RAMP_UP_HIGH ++ until 0x18, then -> HOLD_HIGH
//   HOLD_HIGH    dwells on the same clock, then -> RAMP_DOWN_HIGH
//   RAMP_DOWN_HIGH -- until below 0x13, then -> OFF, counter reset to 1
//
// So there are two independent ramps over two frame bands - 10..18 and 19..24 -
// each with the same rise/hold/fall shape, and OFF is where both land.
GZ_ENUM_BEGIN(SbiHlRowState)
    HLROW_OFF = 0,
    HLROW_IDLE_CYCLE = 1,
    HLROW_RAMP_UP_LOW = 2,
    HLROW_RAMP_DOWN_LOW = 3,
    HLROW_RAMP_UP_HIGH = 4,
    HLROW_RAMP_DOWN_HIGH = 5,
    HLROW_HOLD_HIGH = 6,
    HLROW_HOLD_LOW = 7
GZ_ENUM_END(SbiHlRowState)

#endif // GRUNTZ_GRUNTZ_SBIHLROWSTATE_H
