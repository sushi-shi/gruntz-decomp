// StreamRecordLoaders.h - the HandleEvent-path serialized record (owner:
// StreamRecordLoaders.cpp; the CTriggerLoadRec sibling with its own field layout).
#ifndef GRUNTZ_GRUNTZ_STREAMRECORDLOADERS_H
#define GRUNTZ_GRUNTZ_STREAMRECORDLOADERS_H

#include <Ints.h>

#include <Gruntz/SerialArchive.h> // CSerialArchive (a typedef - a bare fwd decl
                                  // would re-declare a DISTINCT class)

// The serialized event record: two raw dwords, one plain name ref (m_8), one raw
// dword, five bounds-checked type-table index refs (m_10..m_20), two trailing raw
// dwords (m_48/m_4c). Load @0x09c650 (__thiscall, ret 4).
struct CEventLoadRec {
    i32 Load(CSerialArchive* s);

    i32 m_0, m_4; // +0x00,+0x04  raw
    void* m_8;    // +0x08  name ref
    i32 m_c;      // +0x0c  raw
    void* m_10;   // +0x10  indexed type ref
    void* m_14;   // +0x14  indexed type ref
    void* m_18;   // +0x18  indexed type ref
    void* m_1c;   // +0x1c  indexed type ref
    void* m_20;   // +0x20  indexed type ref
    char m_pad24[0x48 - 0x24];
    i32 m_48, m_4c; // +0x48,+0x4c  raw
};

#endif // GRUNTZ_GRUNTZ_STREAMRECORDLOADERS_H
