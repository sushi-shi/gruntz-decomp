#include <rva.h>

#include <Gruntz/ActReg.h>

#include <new>

// @early-stop
// The PMF construction walk is complete. VC5 retains its dead cursor initializer
// like retail, but colors it eax instead of edx and schedules the return move last.
template<> RVA(0x00008710, 0x2b)
zDArray<CActHandler>::zDArray(i32 lo, i32 hi)
    : _zdvec(sizeof(CActHandler), lo, hi, ZVecNoScratch()) {
    for (CActHandler* cursor = AsElem(m_alloc); cursor < AsElem(m_alloc) + m_grown; ++cursor) {
        new (cursor) CActHandler;
    }
}

template<> RVA(0x00008750, 0x15)
zDArray<CActHandler>::~zDArray() {
    for (CActHandler* cursor = AsElem(m_base); cursor <= AsElem(m_base) + (m_hi - m_lo); ++cursor) {
        cursor->~CActHandler();
    }
}

RVA_COMPGEN(0x00008780, 0x1e, ??_G?$zDArray@P8CUserLogic@@AEHXZ@@UAEPAXI@Z)
