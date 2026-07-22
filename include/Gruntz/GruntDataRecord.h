#ifndef SRC_GRUNTZ_GRUNTDATARECORD_H
#define SRC_GRUNTZ_GRUNTDATARECORD_H

#include <rva.h>

#include <Gruntz/SerialArchive.h> // the shared CFileMemBase stream (Read @+0x2c / Write @+0x30)

struct GruntDataRecord {
    char* m_str[5];  // +0x00..+0x10  five owned name strings
    char m_14[0x10]; // +0x14
    char m_24[0x10]; // +0x24
    char m_34[0x10]; // +0x34
    char m_44[0x4];  // +0x44
    char m_48[0x20]; // +0x48..+0x67 (record stride 0x68)

    // Write the five names (as fixed 0x80 fields) + the four fixed blocks through
    // `ar`; returns 0 if `ar` is null, else 1. (0x56da0, __thiscall, 1 stdcall arg.)
    i32 SerializeStrings(CFileMemBase* ar);
    // The read counterpart (0x56eb0): read each fixed 0x80 name field into a temp
    // and assign it to the owned CString member (CString::operator=), then read the
    // four fixed blocks back verbatim. Returns 0 if `ar` is null, else 1.
    i32 DeserializeStrings(CFileMemBase* ar);
};
SIZE_UNKNOWN();

#endif // SRC_GRUNTZ_GRUNTDATARECORD_H
