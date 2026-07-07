// AniElement.h - the 0x28-byte 'ANI' animation element (primary vftable
// @0x5efba8 = ??_7CAniElementObj) cataloged by CDDrawSubMgrAni. It owns a
// CObArray of 0x34-byte frame records (real class CAniRecord, vtable @0x5f02c0)
// plus a name buffer and a scale.
//
// ONE class, ONE header (folded from the former CAniElement.h + CAniElementCollection.h
// dual-def). vtable_hierarchy confirms CAniElementObj : CObject vtbl@0x1efba8 (5 slots,
// slot1 dtor override 0x152e10) - a REAL polymorphic CObject-derived class. The
// frameless-leaf methods (AtChecked/Build/Configure) live in CAniElement.cpp, the /GX
// LoadFile in CAniElementEh.cpp, and the /GX teardown (~CAniElement/DeleteAll) in
// CAniElementCollection.cpp - a deliberate frameless/EH TU split, not two objects.
//
// Layout (offsets/sizes load-bearing; field NAMES are placeholders):
//   +0x00  vptr (CObject grand-base; auto-stamped by cl in the ctor/dtor)
//   +0x04  m_flags   i32 flags/tag word
//   +0x08  m_records CObArray of CObject* frame records (0x14 bytes, +0x08..+0x1b)
//   +0x1c  m_name    char* name buffer (RezAlloc'd len+2, RezFree'd on teardown)
//   +0x20  m_scale   float, set to 1.0f
//   +0x24  m_total   i32 accumulated frame-size total
// size = 0x28
#ifndef GRUNTZ_CANIELEMENT_H
#define GRUNTZ_CANIELEMENT_H

#include <Ints.h>
#include <Mfc.h>                  // CObject / CObArray (real NAFXCW layout)
#include <Wap32/Object.h>         // CObject - the shared engine grand-base
#include <Gruntz/AniRecordView.h> // shared minimal frame-record view (real: CAniRecord)
#include <rva.h>                  // OVERRIDE

// The per-element frame-record array (engine CObArray @+0x08, vtbl 0x5ed494).
// Derives CObArray purely to reach the protected m_pData / m_nSize that retail
// accesses directly (SetAtGrow takes the current size as the append index; the
// bounds-checked accessor reads both). Its member subobject teardown in
// ~CAniElement is the shared MFC ~CObArray (0x1b561c) folded in with a /GX cleanup.
class CAniRecordArray : public CObArray {
public:
    using CObArray::m_nSize;
    using CObArray::m_pData;
};

// arg2 of the builder: a parsed-source descriptor. m_flags @+0x08, m_count @+0x0c,
// m_namelen @+0x10, then the name bytes + record stream start at +0x20.
struct CAniSource {
    char m_pad00[0x8];
    i32 m_flags;   // +0x08 flags (or'd into the element's m_flags)
    i32 m_count;   // +0x0c record count
    i32 m_namelen; // +0x10 name length in bytes
    char m_pad14[0xc];
    char m_data[1]; // +0x20 name bytes followed by the record stream
};

// The 0x34-byte frame record (vtable @0x5f02c0) the builder catalogs is the real
// CAniRecord (src/Gruntz/AniRecord.cpp); callers dispatch through the shared minimal
// polymorphic CAniRecordView (include/Gruntz/AniRecordView.h).

class CAniElement : public CObject {
public:
    virtual ~CAniElement() OVERRIDE;          // 0x152e30 (CAniElementCollection.cpp)
    ::CObject* AtChecked_06b270(i32 i) const; // 0x06b270 (MFC ::CObject array element)
    i32 Build_165460(void* ctx, CAniSource* src, i32 flags); // 0x165460
    i32 Configure_1655c0(void* ctx, void* entry, i32 flags); // 0x1655c0
    i32 LoadFile_165620(void* ctx, void* filename, i32 a3);  // 0x165620 (eh TU)
    void DeleteAll();                                        // 0x165730 (CAniElementCollection.cpp)

    i32 m_flags;               // +0x04
    CAniRecordArray m_records; // +0x08  (0x14 bytes)
    char* m_name;              // +0x1c
    float m_scale;             // +0x20
    i32 m_total;               // +0x24
}; // size = 0x28

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_CANIELEMENT_H
