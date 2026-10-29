#ifndef GRUNTZ_GRUNTPOWEREDSTATEMACROS_H
#define GRUNTZ_GRUNTPOWEREDSTATEMACROS_H

#define RESET_GRUNT_COMBAT_STATE(grunt)                                                            \
    grunt->m_busy = false;                                                                         \
    grunt->m_attackWindupActive = false;                                                           \
    grunt->m_attackQueued = false;                                                                 \
    grunt->m_inCombat = false;                                                                     \
    grunt->ResetIdleAnimation(1, 0, 0);

#endif // GRUNTZ_GRUNTPOWEREDSTATEMACROS_H
