#ifndef GRUNTZ_GRUNTCOMBATCLOCKINLINE_H
#define GRUNTZ_GRUNTCOMBATCLOCKINLINE_H

#include <Bute/ButeMgr.h>
#include <Gruntz/Grunt.h>
#include <Rez/FrameClock.h>

inline void ArmGruntCombatTimeout(CGrunt* grunt) {
    grunt->m_combatTimeoutLo =
        static_cast<i32>(g_buteMgr.GetDword("Grunt", "CombatTimeout", 0x1388));
    grunt->m_combatTimeoutHi = 0;
    grunt->m_combatClockLo = static_cast<i32>(g_frameTime);
    grunt->m_combatClockHi = 0;
}

#endif // GRUNTZ_GRUNTCOMBATCLOCKINLINE_H
