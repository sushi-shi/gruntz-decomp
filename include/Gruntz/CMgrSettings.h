// CMgrSettings.h - a small persisted settings record (0x3c bytes) whose Serialize
// (0x109e00) streams three ints + five doubles through a CArchive-like stream,
// then round-trips a named-object reference (a 0x80 name + an index) through the
// game registry: READ resolves name->record via CMapStringToOb::Lookup and indexes
// the record's bounded element array into m_38; WRITE re-derives the name+index
// from m_38 via CDDrawWorkerRegistry::AnyValueMatches and writes them back.
//
// Same registry round-trip shape as CSerialObjRef/the g_gameReg singleton; field
// names are placeholders (offsets + code bytes load-bearing).
#ifndef GRUNTZ_CMGRSETTINGS_H
#define GRUNTZ_CMGRSETTINGS_H

#include <Mfc.h> // real MFC CMapStringToOb / CObject (Lookup 0x1b8008, reloc-masked)
#include <Gruntz/CGameRegistry.h>
#include <Gruntz/SerialArchive.h> // shared CSerialArchive stream (Read @ +0x2c / Write @ +0x30)
#include <Ints.h>
#include <rva.h>

// The registry leaf reached as g_gameReg->m_world->m_10: a CDDrawWorkerRegistry with
// the name map at +0x10 (read path) and the reverse name+index probe (write path).
class CDDrawWorkerRegistry {
public:
    char _00[0x10];
    CMapStringToOb m_10;                                          // +0x10
    i32 AnyValueMatches_155630(i32 obj, i32 nameBuf, i32 idxPtr); // 0x155630
};

// The record CMapStringToOb::Lookup yields, viewed as a bounded element array.
struct CMgrLookupRec {
    char _00[0x14];
    void** m_14; // +0x14 element array
    char _18[0x64 - 0x18];
    i32 m_64; // +0x64 lo index (inclusive)
    i32 m_68; // +0x68 hi index (inclusive)
};

// g_gameReg singleton chain (CGameRegistry* @ RVA 0x24556c; +0x30 active holder; its
// +0x10 is the registry leaf).
struct CMgrActiveHolder {
    char _00[0x10];
    CDDrawWorkerRegistry* m_10; // +0x10
};
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// Per-serialize round counter the archive bumps.
DATA(0x00229ad0)
extern i32 g_serialCount;

// The settings record itself.
class CMgrSettings {
public:
    i32 Serialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4); // 0x109e00

    i32 m_00, m_04, m_08, m_0c;          // +0x00..+0x0c (m_0c unstreamed)
    double m_10, m_18, m_20, m_28, m_30; // +0x10..+0x30
    void* m_38;                          // +0x38 resolved object reference
};

#endif // GRUNTZ_CMGRSETTINGS_H
