#ifndef GRUNTZ_SERIALRECORDS_H
#define GRUNTZ_SERIALRECORDS_H

#include <rva.h>

#include <Clock64.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>
#include <Io/FileMem.h>

struct CGameObject;

struct CPairRecord {
    Clock64 m_start;
    Clock64 m_duration;
    i32 Serialize(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d);
};
SIZE(0x10);

inline void SerBandPair(CFileMemBase* ar, SerialMode mode, void* band) {
    char* p = static_cast<char*>(band);
    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            ar->Read(p, 8);
            ar->Read(p + 8, 8);
        }
    } else {
        ar->Write(p, 8);
        ar->Write(p + 8, 8);
    }
}

#endif // GRUNTZ_SERIALRECORDS_H
