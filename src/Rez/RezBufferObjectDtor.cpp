#include <Ints.h>
#include <Rez/RezAlloc.h> // RezAlloc/RezFree
#include <Wap32/Object.h> // CObject - the shared engine grand-base
#include <rva.h>
#include <Mfc.h>                 // CArchive (Serialize's arg)
#include <string.h>              // memset/memcpy -> rep stos/movs in the inlined SetSize
#include <new>                   // placement new (ConstructElements' per-element ctor)
#include <Rez/RezBufferObject.h> // RezElem40 (the 0x28 CArray element type)

static inline void ConstructRezElems(RezElem40* p, i32 n) {
    memset(p, 0, n * sizeof(RezElem40));
    for (; n--; p++) {
        ::new (static_cast<void*>(p)) RezElem40;
    }
}

RVA(0x0017f330, 0x51)
CRezBufferObject::~CRezBufferObject() {
    if (m_pData) {
        ::operator delete(m_pData);
    }
}

// ---------------------------------------------------------------------------
// 0x17f130 - CRezBufferObject::Serialize (slot 2): the MFC CArray<40-byte>::Serialize
// (SetSize inlined). Storing: WriteCount then Write the raw 0x28-stride block; loading:
// ReadCount, resize (alloc / grow-with-copy / shrink-in-place per the m_nSize/8 grow
// heuristic) and ConstructElements the new records, then Read the block. Twin of
// CFaderArray::Serialize (0x17e2a0) but with a non-trivial 0x28-byte element.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (serialize family, ~same as CFaderArray/TArray): the branch senses,
// SetSize load/grow/shrink logic, the 0x28 element stride, ConstructElements
// (inlined in-place / called on grow) and Read/Write are byte-faithful, but retail
// pins this->ebx / ar->esi while cl lands different colours through the realloc lea
// chains. Not source-steerable.
RVA(0x0017f130, 0x1ce)
void CRezBufferObject::Serialize(CArchive& ar) {
    if (ar.IsStoring()) {
        ar.WriteCount(m_nSize);
    } else {
        i32 n = ar.ReadCount();
        if (n == 0) {
            if (m_pData != 0) {
                ::operator delete(m_pData);
                m_pData = 0;
            }
            m_nMaxSize = 0;
            m_nSize = 0;
        } else if (m_pData == 0) {
            m_pData = static_cast<RezElem40*>(RezAlloc(n * sizeof(RezElem40)));
            ConstructRezElems(m_pData, n);
            m_nMaxSize = n;
            m_nSize = n;
        } else if (n <= m_nMaxSize) {
            if (n > m_nSize) {
                ConstructRezElems(&m_pData[m_nSize], n - m_nSize);
            }
            m_nSize = n;
        } else {
            i32 grow = m_nGrowBy;
            if (grow == 0) {
                grow = m_nSize / 8;
                if (grow < 4) {
                    grow = 4;
                } else if (grow > 0x400) {
                    grow = 0x400;
                }
            }
            i32 newMax = m_nMaxSize + grow;
            if (n >= newMax) {
                newMax = n;
            }
            RezElem40* nd = static_cast<RezElem40*>(RezAlloc(newMax * sizeof(RezElem40)));
            memcpy(nd, m_pData, m_nSize * sizeof(RezElem40));
            ConstructRezElems(&nd[m_nSize], n - m_nSize);
            ::operator delete(m_pData);
            m_pData = nd;
            m_nSize = n;
            m_nMaxSize = newMax;
        }
    }
    if (ar.IsStoring()) {
        ar.Write(m_pData, m_nSize * sizeof(RezElem40));
    } else {
        ar.Read(m_pData, m_nSize * sizeof(RezElem40));
    }
}
