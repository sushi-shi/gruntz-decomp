#ifndef GRUNTZ_CANIELEMENT_H
#define GRUNTZ_CANIELEMENT_H

#include <Ints.h>
#include <Mfc.h>                  // CObject / CObArray (real NAFXCW layout)
#include <Wap32/Object.h>         // CObject - the shared engine grand-base
#include <Gruntz/AniRecordView.h> // shared minimal frame-record view (real: CAniRecord)
#include <rva.h>                  // OVERRIDE

typedef CObArray CAniRecordArray;

struct CAniSource {
    char m_pad00[0x8];
    i32 m_flags;   // +0x08 flags (or'd into the element's m_flags)
    i32 m_count;   // +0x0c record count
    i32 m_namelen; // +0x10 name length in bytes
    char m_pad14[0xc];
    char m_data[1]; // +0x20 name bytes followed by the record stream
};

class CAniElement : public CObject {
public:
    // Inline ctor (retail inlines it at the CDDrawSubMgrLeaf ANI factory new-sites:
    // base stamp, m_records CObArray-construct @+0x08 via the NAFXCW ctor, own
    // stamp, zero m_flags/m_name). Was the DDrawSubMgrLeaf.cpp-local ctor-shape
    // view CAniElementObj (m_04/CAniElemSub/m_1c) - one class, one def.
    CAniElement() {
        m_flags = 0;
        m_name = 0;
    }
    virtual ~CAniElement() OVERRIDE;          // 0x152e30 (DDrawSubMgrLeaf.cpp)
    ::CObject* AtChecked(i32 i) const; // 0x06b270 (MFC ::CObject array element)
    i32 Build(void* ctx, CAniSource* src, i32 flags); // 0x165460
    i32 Configure(void* ctx, void* entry, i32 flags); // 0x1655c0
    i32 LoadFile(void* ctx, void* filename, i32 a3);  // 0x165620 (eh TU)
    void DeleteAll();                                        // 0x165730 (CAniElementCollection.cpp)

    i32 m_flags;               // +0x04
    CAniRecordArray m_records; // +0x08  (0x14 bytes)
    char* m_name;              // +0x1c
    float m_scale;             // +0x20
    i32 m_total;               // +0x24
}; // size = 0x28
SIZE(CAniElement, 0x28);
VTBL(CAniElement, 0x001efba8); // ??_7 (5 slots; slot 1 = cl-auto ??_G @0x152e10)

#endif // GRUNTZ_CANIELEMENT_H
