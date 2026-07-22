#pragma once
#include <Ints.h>
#include <rva.h>
#include <Wap32/Object.h> // CObject base (pulls the single MFC ::CObject + CArchive)

struct RezElem40 {
    RezElem40(); // 0x17e300 (declared-only; reloc-masked)
    char m_b[0x28];
};
SIZE(0x28);

struct CRezBufferObject : public CObject {
    RezElem40* m_pData; // +0x04  heap buffer (mesh-record array)
    i32 m_nSize;        // +0x08
    i32 m_nMaxSize;     // +0x0c
    i32 m_nGrowBy;      // +0x10
    // Inline ctor (the CObArray-default shape): retail inlines the vptr stamp +
    // the four zero stores at the embedding ctor (CFaderMesh @0x17e940). Store
    // order (m_pData, m_nGrowBy, m_nMaxSize, m_nSize) is the retail one.
    CRezBufferObject() {
        m_pData = 0;
        m_nGrowBy = 0;
        m_nMaxSize = 0;
        m_nSize = 0;
    }
    virtual ~CRezBufferObject() OVERRIDE;          // 0x17f330
    virtual void Serialize(CArchive& ar) OVERRIDE; // slot 2  0x17f130
    // 0x17f390 - the out-of-line CArray<RezElem40>::SetSize (alloc / grow-with-copy /
    // shrink-in-place around RezAlloc/RezFree). Its only caller is CFaderMesh::ApplyInit
    // (0x17ea00, which grows THIS buffer at CFaderMesh+0x58) - so it is this class's
    // method, not a free "CArrayE40" the fader TU invented. Body in Fader.cpp.
    void SetSize(i32 nNewSize, i32 nGrowBy);
};
SIZE_UNKNOWN();
