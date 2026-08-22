#ifndef GRUNTZ_GRUNTPOWEREDSTATEMACROS_H
#define GRUNTZ_GRUNTPOWEREDSTATEMACROS_H

// Clear the four powered-up flags and restart the entrance animation.  The
// receiver-only twin (RESET_GRUNT_POWERED_STATE with no argument) was folded
// onto this form by passing `this` at its 29 sites, 2026-08-22, byte-neutral.
#define RESET_GRUNT_POWERED_STATE(grunt)                                                           \
    grunt->m_entranceActive = 0;                                                                   \
    grunt->m_combatActive = 0;                                                                     \
    grunt->m_neighborValid = 0;                                                                    \
    grunt->m_poweredUp = 0;                                                                        \
    grunt->ResetEntranceAnimation(1, 0, 0);

#define RESET_CURRENT_GRUNT_POWERED_STATE                                                          \
    this->m_entranceActive = 0;                                                                    \
    this->m_combatActive = 0;                                                                      \
    this->m_neighborValid = 0;                                                                     \
    this->m_poweredUp = 0;                                                                         \
    ResetEntranceAnimation(1, 0, 0);

#endif // GRUNTZ_GRUNTPOWEREDSTATEMACROS_H
