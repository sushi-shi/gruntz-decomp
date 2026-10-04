#ifndef GRUNTZ_GRUNTZ_BOOTYSTATEMACROS_H
#define GRUNTZ_GRUNTZ_BOOTYSTATEMACROS_H

#define STAT(getter, field)                                                                        \
    ((m_initOnce != false && g_gameReg->GetGameStats()->IsCurrentAreaComplete() != false)          \
         ? g_gameReg->GetGameStats()->getter()                                                     \
         : g_gameReg->GetGameStats()->field)

#endif // GRUNTZ_GRUNTZ_BOOTYSTATEMACROS_H
