#ifndef GRUNTZ_GRUNTCOMBATCLOCKINLINE_H
#define GRUNTZ_GRUNTCOMBATCLOCKINLINE_H

#include <Bute/ButeMgr.h>
#include <Gruntz/Grunt.h>
#include <Rez/FrameClock.h>

inline void ArmGruntCombatTimeout(CGrunt* grunt) {
    grunt->m_combatTiming.Start(g_buteMgr.GetDword("Grunt", "CombatTimeout", 0x1388));
}

#endif // GRUNTZ_GRUNTCOMBATCLOCKINLINE_H
