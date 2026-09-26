#ifndef GRUNTZ_GRUNTCOMBATCLOCKINLINE_H
#define GRUNTZ_GRUNTCOMBATCLOCKINLINE_H

#include <Bute/ButeMgr.h>
#include <Gruntz/Grunt.h>
#include <Rez/FrameClock.h>

inline void ArmGruntCombatTimeout(CGrunt* grunt) {
    grunt->m_combatTiming.m_intervalLo =
        static_cast<i32>(g_buteMgr.GetDword("Grunt", "CombatTimeout", 0x1388));
    grunt->m_combatTiming.m_intervalHi = 0;
    grunt->m_combatTiming.m_startLo = static_cast<i32>(g_frameTime);
    grunt->m_combatTiming.m_startHi = 0;
}

#endif // GRUNTZ_GRUNTCOMBATCLOCKINLINE_H
