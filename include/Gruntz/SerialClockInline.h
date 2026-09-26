#ifndef GRUNTZ_GRUNTZ_SERIALCLOCKINLINE_H
#define GRUNTZ_GRUNTZ_SERIALCLOCKINLINE_H

#include <Gruntz/ClockInterval.h>
#include <Gruntz/SerialClockMacros.h>
#include <Io/FileMem.h>

static inline void SerializeClockPair(CFileMemBase* ar, SerialMode mode, ClockInterval* timer) {
    SERIALIZE_CLOCK_PAIR(ar, mode, timer->m_start, timer->m_interval);
}

#endif // GRUNTZ_GRUNTZ_SERIALCLOCKINLINE_H
