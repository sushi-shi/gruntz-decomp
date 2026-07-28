#ifndef GRUNTZ_GRUNTZ_ACTREG_H
#define GRUNTZ_GRUNTZ_ACTREG_H

#include <Bute/ButeTree.h> // CVariantSlot complete (ResolveEntry calls its Set)

#include <rva.h>

#include <Gruntz/UserLogic.h>
#include <Wap32/ZVec.h>
#include <Wap32/zBitVec.h> // GetRetAddr and the shared allocation-error state

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
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    m_errSink->Set(this, item, 0xc);
    return AsElem(m_spare);
}

// The same accessor with the grow-fail tail left OUTLINED (`zErrHandling::Report`,
// 0x34960) instead of expanded - retail's shape at every act-table lookup inside a
// TWO-key registrar. See the ResolveEntryCallReport note in <Wap32/ZVec.h>.
template<class T> inline T* zDArray<T>::ResolveEntryCallReport(i32 id) {
    char* r;
    m_grown = 0;
    if (id >= m_lo && id <= m_hi) {
        r = m_base + (id - m_lo) * m_stride;
    } else if (GrowTo(id, 0)) {
        r = m_base + (id - m_lo) * m_stride;
    } else {
        Report(g_projActCache, 0xc);
        r = m_spare;
    }
    return AsElem(r);
}

#endif // GRUNTZ_GRUNTZ_ACTREG_H
