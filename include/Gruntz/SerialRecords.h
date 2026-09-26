#ifndef GRUNTZ_SERIALRECORDS_H
#define GRUNTZ_SERIALRECORDS_H

#include <rva.h>

#include <Gruntz/ClockInterval.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>
#include <Io/FileMem.h>

inline void SerBandPair(CFileMemBase* ar, SerialMode mode, ClockInterval* band) {
    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            ar->Read(&band->m_start, sizeof(band->m_start));
            ar->Read(&band->m_interval, sizeof(band->m_interval));
        }
    } else {
        ar->Write(&band->m_start, sizeof(band->m_start));
        ar->Write(&band->m_interval, sizeof(band->m_interval));
    }
}

#endif // GRUNTZ_SERIALRECORDS_H
