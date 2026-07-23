#ifndef GRUNTZ_GRUNTZ_STREAMRECORDLOADERS_H
#define GRUNTZ_GRUNTZ_STREAMRECORDLOADERS_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/SerialArchive.h> // CFileMemBase (a typedef - a bare fwd decl

struct CEventLoadRec {
    i32 Load(CFileMemBase* s);

    i32 m_0, m_4;            // +0x00,+0x04  raw
    class CDDrawWorker* m_8; // +0x08  the name-looked-up registry sprite (CDDrawWorker)
    i32 m_c;                 // +0x0c  raw
    class CImage* m_10;      // +0x10  indexed frame (items.GetAt bounds-checked)
    class CImage* m_14;      // +0x14  indexed frame
    class CImage* m_18;      // +0x18  indexed frame
    class CImage* m_1c;      // +0x1c  indexed frame
    class CImage* m_20;      // +0x20  indexed frame
    char m_pad24[0x48 - 0x24];
    i32 m_48, m_4c; // +0x48,+0x4c  raw
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_STREAMRECORDLOADERS_H
