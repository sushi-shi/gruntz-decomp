#include <Ints.h>
#include <Rez/RezAlloc.h> // RezAlloc/RezFree
#include <Wap32/Object.h> // CObject - the shared engine grand-base
#include <rva.h>
#include <Mfc.h>    // CArchive (Serialize's arg)
#include <string.h> // memset/memcpy -> rep stos/movs in the inlined SetSize
// <new.h>, NOT <new>: the C++ <new> declares `operator delete(void*) throw()`, and a
// nothrow delete makes cl drop the /GX unwind frame retail's ~CRezBufferObject has.
#include <new.h> // placement new (ConstructRezElems' per-element ctor) + throwing delete
#include <Rez/RezBufferObject.h> // RezElem40 (the 0x28 CArray element type)
#include <Utils/RecordFill.h>    // ZeroRecords (0x17f500) - the realloc arm's tail eraser

static inline void ConstructRezElems(RezElem40* p, i32 n) {
    memset(p, 0, n * sizeof(RezElem40));
    for (; n--; p++) {
        ::new (p) RezElem40;
    }
}

RVA(0x0017f300, 0x3)
RezElem40::RezElem40() {}

// (ex-wall, RETIRED 2026-08-01 - 99.4%, code bytes exact. The dtor was missing retail's
// /GX unwind frame because this TU included <new> for placement new: the C++ <new>
// declares `operator delete(void*) throw()`, and a nothrow delete lets cl drop the
// base-subobject unwind state entirely. <new.h> supplies placement new WITHOUT the
// nothrow spec, and the frame comes back byte-for-byte. Only the __except_list reloc
// name differs from retail's bare fs:0.)
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
            // fresh block: retail zeroes it and stops - no per-element ctor loop here
            memset(m_pData, 0, n * sizeof(RezElem40));
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
            i32 newMax;
            if (n < m_nMaxSize + grow) {
                newMax = m_nMaxSize + grow;
            } else {
                newMax = n;
            }
            RezElem40* nd = static_cast<RezElem40*>(RezAlloc(newMax * sizeof(RezElem40)));
            memcpy(nd, m_pData, m_nSize * sizeof(RezElem40));
            // realloc arm: retail calls the out-of-line record eraser, no ctor loop
            ZeroRecords(&nd[m_nSize], n - m_nSize);
            ::operator delete(m_pData);
            m_pData = nd;
            m_nSize = n;
            m_nMaxSize = newMax;
        }
    }
    RezElem40* data = m_pData;
    i32 cnt = m_nSize;
    if (ar.IsStoring()) {
        ar.Write(data, cnt * sizeof(RezElem40));
    } else {
        ar.Read(data, cnt * sizeof(RezElem40));
    }
}
