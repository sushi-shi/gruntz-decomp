#ifndef GRUNTZ_GRUNTZ_STAMINAPCT_H
#define GRUNTZ_GRUNTZ_STAMINAPCT_H

#include <Enums.h>

// CGrunt::m_stamina is a RECHARGE PERCENTAGE, 0 to 100, not a hit-point pool.
//
// CGrunt proves both the scale and the top: while `m_stamina < STAMINA_FULL` it
// recomputes the value as elapsed-over-downtime times a bute scale, and snaps
// it to STAMINA_FULL the moment the attack downtime has fully elapsed. Every
// other site is the same test - `m_stamina >= STAMINA_FULL` gates whether a
// grunt may act, and the constructor and GruntCombat set it to 0 and back to
// full.
//
// The value was spelled BOTH ways before this header: 0x64 at 22 sites and 100
// at 13, which is the usual sign of a constant nobody had named.
GZ_ENUM_CONST_BEGIN(StaminaPct)
    STAMINA_EMPTY = 0,
    // The one intermediate mark anything tests against, in the
    // `m_stamina <= STAMINA_HALF` low-stamina branch.
    STAMINA_HALF = 50,
    STAMINA_FULL = 100
GZ_ENUM_CONST_END(StaminaPct)

#endif // GRUNTZ_GRUNTZ_STAMINAPCT_H
