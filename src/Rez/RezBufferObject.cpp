#include <rva.h>

#include <Rez/RezBufferObject.h>

#include <Mfc.h>

#include <Ints.h>
#include <Utils/RecordFill.h>
#include <Wap32/Object.h>

#include <new.h>
#include <string.h>

// MFC CArray's storage idiom: a RAW BYTE buffer, elements built later by the
// placement-new loop below. Retail allocates exactly n*0x28 with no ??0RezElem40
// after `call ??2`, so this is NOT `new RezElem40[n]` (that emits a vector-ctor
// loop + a /GX frame). See docs/patterns/msvc5-has-no-array-new-read-the-vector-ctor.md.
static inline void ConstructRezElems(RezElem40* p, i32 n) {
    memset(p, 0, n * sizeof(RezElem40));
    for (; n--; p++) {
        ::new (p) RezElem40;
    }
}

// @early-stop
RVA(0x0017f130, 0x1ce)
void CRezBufferObject::Serialize(CArchive& ar) {
    // Reserve raw capacity: MFC-style growth constructs only newly materialized elements.
    if (ar.IsStoring()) {
        ar.WriteCount(m_nSize);
    } else {
        i32 n = ar.ReadCount();
        if (n == 0) {
            if (m_pData != NULL) {
                delete[] m_pData;
                m_pData = NULL;
            }
            m_nMaxSize = 0;
            m_nSize = 0;
        } else if (m_pData == NULL) {
            m_pData = static_cast<RezElem40*>(::operator new(n * sizeof(RezElem40)));

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
            RezElem40* nd = static_cast<RezElem40*>(::operator new(newMax * sizeof(RezElem40)));
            memcpy(nd, m_pData, m_nSize * sizeof(RezElem40));

            ZeroRecords(&nd[m_nSize], n - m_nSize);
            delete[] m_pData;
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
RVA(0x0017f300, 0x3)
RezElem40::RezElem40() {}

// ??1CRezBufferObject / ??_GCRezBufferObject are pinned in Fader.cpp: the dtor is
// inline-in-header, so cl emits both COMDATs in the TU that expands the ctor
// (fader, via CFaderMesh::m_meshBuf) and none here.

// @early-stop
RVA(0x0017f390, 0x164)
void CRezBufferObject::SetSize(i32 nNewSize, i32 nGrowBy) {
    // Reserve raw capacity: MFC-style growth constructs only newly materialized elements.
    if (nGrowBy != -1) {
        m_nGrowBy = nGrowBy;
    }
    if (nNewSize == 0) {
        if (m_pData != NULL) {
            delete[] m_pData;
            m_pData = NULL;
        }
        m_nSize = m_nMaxSize = 0;
    } else if (m_pData == NULL) {
        m_pData = static_cast<RezElem40*>(::operator new(nNewSize * sizeof(RezElem40)));
        memset(m_pData, 0, nNewSize * sizeof(RezElem40));
        m_nSize = m_nMaxSize = nNewSize;
    } else if (nNewSize <= m_nMaxSize) {
        if (nNewSize > m_nSize) {
            memset(&m_pData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(RezElem40));
        }
        m_nSize = nNewSize;
    } else {
        i32 grow = m_nGrowBy;
        if (grow == 0) {
            grow = m_nSize / 8;
            if (grow < 4) {
                grow = 4;
            } else if (grow > 1024) {
                grow = 1024;
            }
        }
        i32 nNewMax;
        if (nNewSize < m_nMaxSize + grow) {
            nNewMax = m_nMaxSize + grow;
        } else {
            nNewMax = nNewSize;
        }
        RezElem40* pNewData = static_cast<RezElem40*>(::operator new(nNewMax * sizeof(RezElem40)));
        memcpy(pNewData, m_pData, m_nSize * sizeof(RezElem40));
        memset(&pNewData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(RezElem40));
        delete[] m_pData;
        m_pData = pNewData;
        m_nSize = nNewSize;
        m_nMaxSize = nNewMax;
    }
}
RVA(0x0017f500, 0x23)
void __stdcall ZeroRecords(void* dst, int count) {
    memset(dst, 0, count * 0x28);
}
