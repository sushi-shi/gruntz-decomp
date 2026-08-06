#define CREZBUFFEROBJECT_OOL_DTOR

#include <rva.h>

#include <Rez/RezBufferObject.h>

#include <Mfc.h>

#include <Ints.h>
#include <Utils/RecordFill.h>
#include <Wap32/Object.h>

#include <new.h>
#include <string.h>

static inline void ConstructRezElems(RezElem40* p, i32 n) {
    memset(p, 0, n * sizeof(RezElem40));
    for (; n--; p++) {
        ::new (p) RezElem40;
    }
}

// @early-stop
RVA(0x0017f130, 0x1ce)
void CRezBufferObject::Serialize(CArchive& ar) {
    if (ar.IsStoring()) {
        ar.WriteCount(m_nSize);
    } else {
        i32 n = ar.ReadCount();
        if (n == 0) {
            if (m_pData != NULL) {
                ::operator delete(m_pData);
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
RVA(0x0017f300, 0x3)
RezElem40::RezElem40() {}

RVA(0x0017f330, 0x51)
CRezBufferObject::~CRezBufferObject() {
    if (m_pData) {
        ::operator delete(m_pData);
    }
}

// @early-stop
