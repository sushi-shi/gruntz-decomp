#ifndef GRUNTZ_GRUNTZ_BOOTYSTATEMACROS_H
#define GRUNTZ_GRUNTZ_BOOTYSTATEMACROS_H

#define STAT(getter, field)                                                                        \
    ((m_initOnce != false && g_gameReg->m_gameStats->m_currentAreaComplete != false)               \
         ? g_gameReg->m_gameStats->getter()                                                        \
         : g_gameReg->m_gameStats->field)

#endif // GRUNTZ_GRUNTZ_BOOTYSTATEMACROS_H
