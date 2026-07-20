#ifndef GRUNTZ_SERIALRECORDS_H
#define GRUNTZ_SERIALRECORDS_H

#include <Ints.h>
#include <rva.h>

class CFileMemBase;
typedef CFileMemBase CSerialArchive;

struct CTriRecord {
    i32 m_0;
    i32 m_4;
    i32 m_8;
    i32 Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d); // 0x3c8f0
};

struct CPairRecord {
    i64 m_0;
    i64 m_8;
    i32 Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d); // 0x58ee0
};

#endif // GRUNTZ_SERIALRECORDS_H
