#include <rva.h>
#include <Ints.h>

#define SBI_OWN_IMAGESETANI_DTOR // this TU supplies the out-of-line ~CSBI_ImageSetAni
#include <Gruntz/SbiDtorChain.h>
// SBI_ImageSetAniEh.cpp - the /GX EH-framed CSBI_ImageSetAni destructor (0x1047f0)
// (C:\Proj\Gruntz). The split off the frameless base TU is matching-neutral (each
// function is RVA-keyed).
//
// 5-level case of docs/patterns/eh-dtor-multilevel-polymorphic-chain.md.
// CSBI_ImageSetAni is the OUT-OF-LINE leaf here (SBI_OWN_IMAGESETANI_DTOR guard)
// but the INLINE base of ~CSBI_StatzTabArrow elsewhere.

RVA(0x001047f0, 0x94)
CSBI_ImageSetAni::~CSBI_ImageSetAni() {
    DtorImageSetAni();
}
