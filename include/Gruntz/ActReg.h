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

template<class T> inline char* zDArray<T>::ResolveEntry(i32 id) {
    m_grown = 0;
    if (id >= m_lo && id <= m_hi) {
        return m_base + (id - m_lo) * m_stride;
    }
    if (GrowTo(id, 0)) {
        return m_base + (id - m_lo) * m_stride;
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    m_errSink->Set(this, reinterpret_cast<i32>(item), 0xc);
    return m_spare;
}

#endif // GRUNTZ_GRUNTZ_ACTREG_H
