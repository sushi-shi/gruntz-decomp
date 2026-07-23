#include <Gruntz/ActReg.h>
#include <rva.h>

template<> RVA(0x00008710, 0x2b)
zDArray<CActHandler>::zDArray(i32 lo, i32 hi)
    : _zdvec(sizeof(CActHandler), lo, hi, reinterpret_cast<void*>(1)) {
    CActHandler* volatile item = reinterpret_cast<CActHandler*>(m_alloc);
}

template<> RVA(0x00008750, 0x15)
zDArray<CActHandler>::~zDArray() {
    CActHandler* volatile item = reinterpret_cast<CActHandler*>(m_base);
}

RVA_COMPGEN(0x00008780, 0x1e, ??_G?$zDArray@P8CUserLogic@@AEHXZ@@UAEPAXI@Z)
