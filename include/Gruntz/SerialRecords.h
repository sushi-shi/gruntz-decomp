#ifndef GRUNTZ_SERIALRECORDS_H
#define GRUNTZ_SERIALRECORDS_H

#include <Ints.h>
#include <Clock64.h>
#include <rva.h>

#include <Io/FileMem.h>
struct CGameObject;

struct CPairRecord {
    Clock64 m_start;
    Clock64 m_duration;
    i32 Serialize(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d);
};
SIZE(0x10);

inline void SerBandPair(CFileMemBase* ar, i32 mode, void* band) {
    char* p = static_cast<char*>(band);
    if (mode != 4) {
        if (mode == 7) {
            ar->Read(p, 8);
            ar->Read(p + 8, 8);
        }
    } else {
        ar->Write(p, 8);
        ar->Write(p + 8, 8);
    }
}

#endif // GRUNTZ_SERIALRECORDS_H
