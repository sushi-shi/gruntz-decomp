#ifndef GRUNTZ_SERIALRECORDS_H
#define GRUNTZ_SERIALRECORDS_H

#include <rva.h>

#include <Gruntz/ClockInterval.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>
#include <Io/FileMem.h>

inline void SerializeClockPair(CFileMemBase* ar, SerialMode mode, ClockInterval* timer) {
    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            ar->Read(&timer->m_start, sizeof(timer->m_start));
            ar->Read(&timer->m_interval, sizeof(timer->m_interval));
        }
    } else {
        ar->Write(&timer->m_start, sizeof(timer->m_start));
        ar->Write(&timer->m_interval, sizeof(timer->m_interval));
    }
}

#endif // GRUNTZ_SERIALRECORDS_H
