#ifndef GRUNTZ_GRUNTZ_ACTREG_H
#define GRUNTZ_GRUNTZ_ACTREG_H

#include <rva.h>

#include <Bute/ButeTree.h>
#include <Gruntz/UserLogic.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

typedef i32 (CUserLogic::*CActHandler)();
typedef zDArray<CActHandler> CActReg;

template<class Tag> struct CActRegPool {
    static CActReg s_table;
};

template<class T> inline T* zDArray<T>::ResolveEntry(i32 id) {
    m_grown = 0;
    if (id >= m_lo && id <= m_hi) {
        return AsElem(m_base + (id - m_lo) * m_stride);
    }
    if (GrowTo(id, 0)) {
        return AsElem(m_base + (id - m_lo) * m_stride);
    }
    char* msg = g_errOutOfMem;
    g_retAddrBreadcrumb = GetRetAddr();
    m_errSink->Set(this, msg, 0xc);
    return AsElem(m_spare);
}

template<class T> inline T* zDArray<T>::ResolveEntryCallReport(i32 id) {
    char* r;
    m_grown = 0;
    if (id >= m_lo && id <= m_hi) {
        r = m_base + (id - m_lo) * m_stride;
    } else if (GrowTo(id, 0)) {
        r = m_base + (id - m_lo) * m_stride;
    } else {
        Report(g_errOutOfMem, 0xc);
        r = m_spare;
    }
    return AsElem(r);
}

#endif // GRUNTZ_GRUNTZ_ACTREG_H
