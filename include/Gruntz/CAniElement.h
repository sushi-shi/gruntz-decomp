// CAniElement.h - the 0x28-byte 'ANI' animation element (primary vftable
// @0x5efba8) cataloged by CDDrawSubMgrAni. It owns a CObArray of 0x34-byte frame
// records (ClassUnknown_39, vtable @0x5f02c0) plus a name buffer and a scale.
//
// Layout (offsets/sizes load-bearing; field NAMES are placeholders):
//   +0x00  vptr (CObject-derived element; not modeled here - none of these three
//          methods touch +0, so no vtable machinery is needed)
//   +0x04  m_04      i32 flags/tag word
//   +0x08  m_records CObArray of CObject* frame records (0x14 bytes, +0x08..+0x1b)
//   +0x1c  m_name    char* name buffer (RezAlloc'd len+2, RezFree'd on teardown)
//   +0x20  m_scale   float, set to 1.0f
//   +0x24  m_total   i32 accumulated frame-size total
// size = 0x28
#ifndef GRUNTZ_CANIELEMENT_H
#define GRUNTZ_CANIELEMENT_H

#include <Ints.h>
#include <Mfc.h> // CObject / CObArray (real NAFXCW layout)

// The per-element frame-record array. Derives CObArray purely to reach the
// protected m_pData / m_nSize that retail accesses directly (SetAtGrow takes the
// current size as the append index; the bounds-checked accessor reads both).
class CAniRecordArray : public CObArray {
public:
    using CObArray::m_nSize;
    using CObArray::m_pData;
};

// arg2 of the builder: a parsed-source descriptor. m_08 flags, m_0c record count,
// m_10 name length, then the name bytes + record stream start at +0x20.
struct CAniSource {
    char m_pad00[0x8];
    i32 m_08;      // +0x08 flags (or'd into the element's m_04)
    i32 m_count;   // +0x0c record count
    i32 m_namelen; // +0x10 name length in bytes
    char m_pad14[0xc];
    char m_data[1]; // +0x20 name bytes followed by the record stream
};

// A 0x34-byte frame record (ClassUnknown_39; vtable @0x5f02c0). Modeled minimally:
// only the fields the builder zeroes/sets, plus its parse entry and a scalar dtor
// (vtable slot+4) for the failure-path teardown. Parse/GetSize live in their own
// cluster (external/no-body); the vtable contents are not modeled, so the builder
// stamps the primary vtable by address (reloc-masked DATA extern).
// DISPOSITION: the REAL class is CAniRecord (src/Gruntz/CAniRecord.cpp, VTBL @0x1f02c0)
// - the same 0x34-byte frame record fully reconstructed there. This is a minimal
// duplicate view kept because CAniRecord is not yet in a shared header; collapsing the
// two is a header-dedup follow-up, not a rename (the identity is already known).
class CAniRecordView {
public:
    virtual void Slot00();                           // +0x00
    virtual i32 ScalarDtor(i32 flag);                // +0x04 scalar-deleting destructor
    i32 Parse_168c60(void* ctx, const char* cursor); // 0x168c60 __thiscall
    i32 GetSize_168e50();                            // 0x168e50 __thiscall
};

class CAniElement {
public:
    CObject* AtChecked_06b270(i32 i) const;                  // 0x06b270
    i32 Build_165460(void* ctx, CAniSource* src, i32 flags); // 0x165460
    i32 Configure_1655c0(void* ctx, void* entry, i32 flags); // 0x1655c0
    i32 LoadFile_165620(void* ctx, void* filename, i32 a3);  // 0x165620 (eh TU)

    void* m_vptr;              // +0x00
    i32 m_04;                  // +0x04
    CAniRecordArray m_records; // +0x08  (0x14 bytes)
    char* m_name;              // +0x1c
    float m_scale;             // +0x20
    i32 m_total;               // +0x24
}; // size = 0x28

#endif // GRUNTZ_CANIELEMENT_H
