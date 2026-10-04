#ifndef GRUNTZ_GRUNTPOWEREDSTATEMACROS_H
#define GRUNTZ_GRUNTPOWEREDSTATEMACROS_H

#define RESET_GRUNT_COMBAT_STATE(grunt)                                                            \
    grunt->m_entranceActive = false;                                                               \
    grunt->m_attackWindupActive = false;                                                           \
    grunt->m_attackQueued = false;                                                                 \
    grunt->m_inCombat = false;                                                                     \
    grunt->ResetEntranceAnimation(1, 0, 0);

#define RESET_CURRENT_GRUNT_COMBAT_STATE                                                           \
    this->m_entranceActive = false;                                                                \
    this->m_attackWindupActive = false;                                                            \
    this->m_attackQueued = false;                                                                  \
    this->m_inCombat = false;                                                                      \
    ResetEntranceAnimation(1, 0, 0);

#endif // GRUNTZ_GRUNTPOWEREDSTATEMACROS_H
