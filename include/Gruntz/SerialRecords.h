#ifndef GRUNTZ_SERIALRECORDS_H
#define GRUNTZ_SERIALRECORDS_H

#include <Ints.h>
#include <rva.h>

#include <Io/FileMem.h>
struct CGameObject;

struct CTriRecord {
    i32 m_0;
    i32 m_4;
    i32 m_8;
    i32 Serialize(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d);
};
SIZE_UNKNOWN();

struct CPairRecord {
    i64 m_0;
    i64 m_8;
    i32 Serialize(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d);
};
SIZE_UNKNOWN();

inline i32 SerTriRecord(void* band, CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    return static_cast<CTriRecord*>(band)->Serialize(ar, tag, c, d);
}
inline i32 SerPairRecord(void* band, CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {

    return static_cast<CPairRecord*>(band)->Serialize(ar, tag, c, d);
}

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
