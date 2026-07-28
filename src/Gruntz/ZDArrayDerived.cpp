#include <Gruntz/ActReg.h>
#include <rva.h>

template<> RVA(0x00008710, 0x2b)
zDArray<CActHandler>::zDArray(i32 lo, i32 hi)
    : _zdvec(sizeof(CActHandler), lo, hi, ZVecNoScratch()) {
    // retail 0x8728: `mov edx,[esi+0x1c]` (m_alloc) stored into the dead `hi`
    // parameter slot and never read - a live-but-unused element cursor. The band is
    // byte-addressed (runtime m_stride), so the cursor's own type IS char*: no cast.
    char* volatile cursor = m_alloc;
}

template<> RVA(0x00008750, 0x15)
zDArray<CActHandler>::~zDArray() {
    // retail 0x8751: `mov eax,[ecx+0x10]` (m_base) into the one `push ecx` local,
    // never read. Same byte-addressed band, same char* cursor - no cast.
    char* volatile cursor = m_base;
}

RVA_COMPGEN(0x00008780, 0x1e, ??_G?$zDArray@P8CUserLogic@@AEHXZ@@UAEPAXI@Z)
