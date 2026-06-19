#include "../rva.h"
// CDDrawPtrCollections.cpp - tomalla-named standalone class in the ddrawmgr surface/page
// manager "Harry Potter" family.  CDDrawPtrCollections (0x948 B, NO vtable) owns two
// CPtrList (+0x47c / +0x498) and one CPtrArray (+0x4b4).  The ctor @0x141cc0
// constructs the three MFC containers with the given block sizes (both lists 0xa),
// clears the scalar fields, and carries a C++ EH frame (/GX) to unwind the
// constructed containers if a later one throws.
//
// Field names are tomalla placeholders; only the OFFSETS + the emitted code
// bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

// Minimal MFC container placeholders — only the ctor symbol + size matter.
// CPtrList: vptr@0 + 6 scalar fields = 0x1c.  CPtrArray: vptr@0 + 4 fields = 0x14.
class CPtrList {
public:
    CPtrList(int nBlockSize);               // @0x1b4867
    ~CPtrList();                            // (invoked on EH unwind)
    char _raw[0x1c - 4];                    // 0x1c incl vptr
};

class CPtrArray {
public:
    CPtrArray();                            // @0x1b4f0b
    ~CPtrArray();                           // (invoked on EH unwind)
    char _raw[0x14 - 4];                    // 0x14 incl vptr
};

// ---------------------------------------------------------------------------
// CDDrawPtrCollections — reconstructed Ctor ONLY.  The class has no vtable (the
// default ctor doesn't stamp one).  Fields from the structure layout in
// structure/managers/ddrawmgr_surface_family.h; only the offsets the ctor
// touches are modeled below.
// ---------------------------------------------------------------------------
class CDDrawPtrCollections {
public:
    CDDrawPtrCollections();

    int        fieldUnknown000;              // +0x00  — zeroed in ctor
    int        fieldUnknown004;              // +0x04  — zeroed in ctor
    char       _pad008[0x47c - 0x08];       // +0x08..0x47b
    CPtrList   m_unknownPtrList1;           // +0x47c  (ctor blockSize=0xa)
    CPtrList   m_unknownPtrList2;           // +0x498  (ctor blockSize=0xa)
    CPtrArray  m_unknownPtrArray;           // +0x4b4  (default ctor)
    char       _pad4C8[0x534 - 0x4c8];      // +0x4c8..0x533
    int        fieldUnknown534;             // +0x534  — zeroed in ctor
    int        fieldUnknown538;             // +0x538  — zeroed in ctor
    char       _pad53C[0x93c - 0x53c];      // +0x53c..0x93b
    int        fieldUnknown93C;             // +0x93c  — zeroed in ctor
    int        fieldUnknown940;             // +0x940  — zeroed in ctor
    int        fieldUnknown944;             // +0x944  — zeroed in ctor
};                                          // 0x948

// ---------------------------------------------------------------------------
// CDDrawPtrCollections::CDDrawPtrCollections  @0x141cc0
// Constructs the two CPtrLists with block size 0xa and the CPtrArray, then
// zeroes all the scalar fields the ctor touches.  /GX: the three MFC container
// ctors may throw, so the compiler emits a C++ EH frame (fs:0) whose try level
// advances as each container is constructed (the target shows try_level updates
// at [esp+0x18]=0 after list1, [esp+0x14]=1 after list2).
// ---------------------------------------------------------------------------
RVA(0x141cc0, 0x84)
CDDrawPtrCollections::CDDrawPtrCollections()
    : m_unknownPtrList1(0xa)
    , m_unknownPtrList2(0xa)
    , m_unknownPtrArray()
{
    fieldUnknown000 = 0;
    fieldUnknown004 = 0;
    fieldUnknown534 = 0;
    fieldUnknown538 = 0;
    fieldUnknown93C = 0;
    fieldUnknown940 = 0;
    fieldUnknown944 = 0;
}
