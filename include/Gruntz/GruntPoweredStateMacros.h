#ifndef GRUNTZ_GRUNTPOWEREDSTATEMACROS_H
#define GRUNTZ_GRUNTPOWEREDSTATEMACROS_H

#define RESET_GRUNT_COMBAT_STATE(grunt)                                                            \
    grunt->m_entranceActive = false;                                                               \
    grunt->m_combatActive = false;                                                                 \
    grunt->m_neighborValid = false;                                                                \
    grunt->m_inCombat = false;                                                                     \
    grunt->ResetEntranceAnimation(1, 0, 0);

#define RESET_CURRENT_GRUNT_COMBAT_STATE                                                           \
    this->m_entranceActive = false;                                                                \
    this->m_combatActive = false;                                                                  \
    this->m_neighborValid = false;                                                                 \
    this->m_inCombat = false;                                                                      \
    ResetEntranceAnimation(1, 0, 0);

#endif // GRUNTZ_GRUNTPOWEREDSTATEMACROS_H
