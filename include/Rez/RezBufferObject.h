#pragma once
#include <Ints.h>
#include <rva.h>
#include <Wap32/Object.h> // CObject base (pulls the single MFC ::CObject + CArchive)

// A 40-byte mesh record - the element type of CRezBufferObject's CObArray (proven by
// Serialize @0x17f130, a 0x28-stride SetSize-grow buffer). Its per-element ctor
// (0x17e300, reloc-masked, declared-only) makes cl emit the `if(p) p->T::T()`
// placement-new guard the retail per-element grow loop shows.
struct RezElem40 {
    RezElem40(); // 0x17e300 (declared-only; reloc-masked)
    char m_b[0x28];
};
SIZE(RezElem40, 0x28);

// The worker IS a CObArray of 40-byte mesh records (proven by Serialize @0x17f130:
// it reads m_pData(+0x04)/m_nSize(+0x08)/m_nMaxSize(+0x0c)/m_nGrowBy(+0x10) and
// SetSize-grows a 0x28-stride buffer). Its dtor RezFrees m_pData. (Was a
// RezBufferObjectDtor.cpp-local view; now the shared canonical.)
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
};
SIZE_UNKNOWN(CRezBufferObject);
