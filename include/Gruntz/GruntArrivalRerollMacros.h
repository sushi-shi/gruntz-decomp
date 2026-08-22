#ifndef GRUNTZ_GRUNTARRIVALREROLLMACROS_H
#define GRUNTZ_GRUNTARRIVALREROLLMACROS_H

#define RESET_GRUNT_ARRIVAL_REROLL_COMPACT                                                         \
    ResetEntranceAnimation(1, 1, 0);                                                               \
    m_arrivalRerollWindowLo = rand() % 0x7530 + 0x7530;                                            \
    m_arrivalRerollWindowHi = 0;                                                                   \
    m_arrivalRerollLo = static_cast<i32>(g_frameTime);                                             \
    m_arrivalRerollHi = 0;

#endif // GRUNTZ_GRUNTARRIVALREROLLMACROS_H
