#ifndef GRUNTZ_SERIALRECORDS_H
#define GRUNTZ_SERIALRECORDS_H

#include <Ints.h>
#include <rva.h>

#include <Io/FileMem.h> // CFileMemBase complete (SerBandPair streams through it)
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

// The 0x10-byte timer/clock BANDS the archive snapshots as two 8-byte blocks. The
// band base is a member address, so the void* parameter keeps retail's single-lea
// shape without a cast at each call.
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
