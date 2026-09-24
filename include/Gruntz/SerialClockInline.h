#ifndef GRUNTZ_GRUNTZ_SERIALCLOCKINLINE_H
#define GRUNTZ_GRUNTZ_SERIALCLOCKINLINE_H

#include <Gruntz/PathHazard.h>
#include <Gruntz/SerialClockMacros.h>
#include <Io/FileMem.h>

static __inline void SerializeClockPair(CFileMemBase* ar, SerialMode mode, i64* pair) {
    SERIALIZE_CLOCK_PAIR(ar, mode, *pair, *(pair + 1));
}

static inline void SerializeClockPair(CFileMemBase* ar, SerialMode mode, CHazardTimer* timer) {
    SERIALIZE_CLOCK_PAIR(ar, mode, timer->m_deadline, timer->m_window);
}

#endif // GRUNTZ_GRUNTZ_SERIALCLOCKINLINE_H
