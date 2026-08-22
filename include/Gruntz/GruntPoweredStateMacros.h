#ifndef GRUNTZ_GRUNTPOWEREDSTATEMACROS_H
#define GRUNTZ_GRUNTPOWEREDSTATEMACROS_H

#define RESET_GRUNT_POWERED_STATE                                                                  \
    m_entranceActive = 0;                                                                          \
    m_combatActive = 0;                                                                            \
    m_neighborValid = 0;                                                                           \
    m_poweredUp = 0;                                                                               \
    ResetEntranceAnimation(1, 0, 0);

#define RESET_GRUNT_POWERED_STATE_FOR(grunt)                                                       \
    grunt->m_entranceActive = 0;                                                                   \
    grunt->m_combatActive = 0;                                                                     \
    grunt->m_neighborValid = 0;                                                                    \
    grunt->m_poweredUp = 0;                                                                        \
    grunt->ResetEntranceAnimation(1, 0, 0);

#endif // GRUNTZ_GRUNTPOWEREDSTATEMACROS_H
