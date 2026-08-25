#ifndef GRUNTZ_GRUNTZ_SBIBELTPHASE_H
#define GRUNTZ_GRUNTZ_SBIBELTPHASE_H

#include <Enums.h>

// Where the status bar's conveyor item is in its journey, as carried by
// CStatusBarMgr::m_machinePhase. Named by RETAIL'S OWN bute keys, which each
// phase reads to set its own frame interval and step size:
//
//   1  idle, and where phase 8 returns to
//   2  "NextItemInMachineTime"      the item waits inside the machine
//   3  "RightMachineSpewingDelay"   the right machine spews it out
//   4  hands over to the fall, setting "FallingItemDelay"
//   5  "FallingItemSpeed" downward  it falls to the belt
//   7  "NextItemSpeed" leftward     it travels the belt to m_machineItemTargetX
//   8  "FallingItemSpeed" downward  it falls off the end, then back to idle
//
// The order is not guesswork: each arm assigns the next phase, so the chain
// 2 -> 3 -> 4 -> 5 -> 7 -> 8 -> 1 is written in the code.
GZ_ENUM_BEGIN(SbiBeltPhase)
    BELT_IDLE = 1,
    BELT_IN_MACHINE = 2,
    BELT_SPEWING = 3,
    BELT_DROP_START = 4,
    BELT_FALLING = 5,
    BELT_TRAVELLING = 7,
    BELT_FALLING_OFF = 8
GZ_ENUM_END(SbiBeltPhase)

#endif // GRUNTZ_GRUNTZ_SBIBELTPHASE_H
