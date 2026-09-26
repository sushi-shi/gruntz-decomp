#ifndef GRUNTZ_GRUNTCOMBATCLOCKINLINE_H
#define GRUNTZ_GRUNTCOMBATCLOCKINLINE_H

#include <Bute/ButeMgr.h>
#include <Gruntz/Grunt.h>
#include <Rez/FrameClock.h>

inline void ArmGruntCombatTimeout(CGrunt* grunt) {
    grunt->m_combatTiming.m_interval.m_lo =
        static_cast<i32>(g_buteMgr.GetDword("Grunt", "CombatTimeout", 0x1388));
    grunt->m_combatTiming.m_interval.m_hi = 0;
    grunt->m_combatTiming.m_start.m_lo = static_cast<i32>(g_frameTime);
    grunt->m_combatTiming.m_start.m_hi = 0;
}

#endif // GRUNTZ_GRUNTCOMBATCLOCKINLINE_H
