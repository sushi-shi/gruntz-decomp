#ifndef GRUNTZ_SERIALRECORDS_H
#define GRUNTZ_SERIALRECORDS_H

#include <Ints.h>
#include <rva.h>

class CFileMemBase;
struct CGameObject;

struct CTriRecord {
    i32 m_0;
    i32 m_4;
    i32 m_8;
    i32 Serialize(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d); // 0x3c8f0
};
SIZE_UNKNOWN();

struct CPairRecord {
    i64 m_0;
    i64 m_8;
    i32 Serialize(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d); // 0x58ee0
};
SIZE_UNKNOWN();

// Retail serialises these records by `lea`-ing the owning class's member BAND and
// calling Serialize on it - the record is a view of those bytes, not a member, so
// the overlay is inherent to the call. Keep it at these two seams.
inline i32 SerTriRecord(void* band, CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    return reinterpret_cast<CTriRecord*>(band)->Serialize(ar, tag, c, d);
}
inline i32 SerPairRecord(void* band, CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    return reinterpret_cast<CPairRecord*>(band)->Serialize(ar, tag, c, d);
}

#endif // GRUNTZ_SERIALRECORDS_H
