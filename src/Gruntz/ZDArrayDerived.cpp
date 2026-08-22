#include <rva.h>

#include <Gruntz/ActReg.h>

#include <new>

// The PMF element type is trivially constructible, so the walk is dead code that
// VC5 keeps only as the homed `first` initializer.
template<> RVA(0x00008710, 0x2b)
zDArray<CActHandler>::zDArray(i32 lo, i32 hi)
    : _zdvec(sizeof(CActHandler), lo, hi, ZVecNoScratch()) {
    CActHandler* first = AsElem(m_alloc);
    for (CActHandler* cursor = first; cursor < first + m_grown; ++cursor) {
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
