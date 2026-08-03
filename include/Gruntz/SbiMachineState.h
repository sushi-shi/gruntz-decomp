#ifndef GRUNTZ_GRUNTZ_SBIMACHINESTATE_H
#define GRUNTZ_GRUNTZ_SBIMACHINESTATE_H

#include <Enums.h>

// The animation state of the status bar's GruntMachine widgets (m_machineA and
// m_machineB). Named by RETAIL'S OWN bute keys - each state sets the frame
// interval it runs at by name, so the key names the state:
//
//   1  "StatusBar" / "LeftMachineSnoozingDelay"      idle
//   2  "StatusBar" / "LeftMachineWakingDelay"        rousing; at frame 0x13 it
//                                                    hands over to the wheel
//   3  "StatusBar" / "LeftMachineTurningWheelDelay"  running
//   4  "StatusBar" / "LeftMachineLeverDelay"         dispensing
//
// NOTE these share the CSbiHlRow storage with SbiHlRowState but are a DIFFERENT
// machine: m_groupSlots run the rise/hold/fall highlight states, m_machineA and
// m_machineB run these. Two domains, one field, told apart by which instance is
// being stepped - which is why CSbiHlRow::m_state is not typed with either.
GZ_ENUM_BEGIN(SbiMachineState)
    MACHINE_SNOOZING = 1,
    MACHINE_WAKING = 2,
    MACHINE_TURNING_WHEEL = 3,
    MACHINE_LEVER = 4
GZ_ENUM_END(SbiMachineState)

#endif // GRUNTZ_GRUNTZ_SBIMACHINESTATE_H
