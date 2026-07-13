// SerialRecords.h - two small fixed-layout records with a CSerialArchive Serialize
// (orphan COMDATs): CTriRecord (3xi32 @0x3c8f0) and CPairRecord (2xi64 @0x58ee0).
// Extracted from TriRecordSerialize.cpp so CGrunt's serialize path reaches them
// cast-free (was the CGruntSubSer generic-serialize view). Names placeholder;
// offsets + code bytes load-bearing.
#ifndef GRUNTZ_SERIALRECORDS_H
#define GRUNTZ_SERIALRECORDS_H

#include <Ints.h>
#include <rva.h>

// The serialize stream is the REAL CFileMemBase (<Gruntz/SerialArchive.h> typedefs
// CSerialArchive onto it); a fwd decl of the OLD placeholder name here would
// re-declare a distinct class and silently out-rank the typedef (MSVC5).
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
