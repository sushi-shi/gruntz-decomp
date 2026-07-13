// SubWidget168080.h - the 0x44-byte sub-widget/grid sibling of CWwdGrid (its own 6-slot
// vtable ??_7SubWidget_168080 @0x1f0310, RTTI-less). Hoisted out of GameLevelMove.cpp
// (where it was a .cpp-local view) because its DESTRUCTOR body lives in the WwdGrid obj:
// the vtable-owner probe binds it - ??_7 @0x1f0310 slot 1 -> the scalar-deleting dtor
// 0x168280 -> the ??1 at 0x1682a0.
//
// That body was previously misbound as CWwdGrid::~CWwdGrid, while the REAL ~CWwdGrid
// (0x168c10, dispatched from ??_7CWwdGrid @0x1f0328 slot 1 via the sdd 0x168bf0) wore a
// fake "duplicate COMDAT copy" placeholder. The two grids are SIBLING classes with two
// distinct vtables, not one class with two dtor copies - MSVC5 keeps exactly one COMDAT
// per mangled name, so a "second copy" of ~CWwdGrid could never have survived the link.
//
// Field names are placeholders; only the offsets + emitted bytes are load-bearing.
#ifndef GRUNTZ_WWD_SUBWIDGET168080_H
#define GRUNTZ_WWD_SUBWIDGET168080_H

#include <Ints.h>
#include <Wap32/Object.h> // CObject grand-base (slots 0/2/3/4)
#include <rva.h>
#include <Win32.h> // RECT (Setup's by-value arg)

struct SubWidget_168080 : public CObject {
    virtual ~SubWidget_168080() OVERRIDE; // [1] +0x04; ??_G 0x168280, ??1 0x1682a0
    virtual void Slot14_168060();         // [5] 0x168060 (declared-only)
    i32 m_4;                              // +0x04
    char m_pad8[0x44 - 8];
    i32 Setup(RECT rc, i32 a, i32 b); // 0x1915c0 (reloc-masked)
    SubWidget_168080() {
        m_4 = 0; // cl auto-stamps &??_7SubWidget_168080 first
    }
};
SIZE(SubWidget_168080, 0x44);
VTBL(SubWidget_168080, 0x001f0310); // ??_7SubWidget_168080 (was g_subVtbl_5f0310)

#endif // GRUNTZ_WWD_SUBWIDGET168080_H
