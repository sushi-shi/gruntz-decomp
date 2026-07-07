#include <rva.h>
#include <Ints.h>

#include <Gruntz/SbiDtorChain.h>
// SBI_StatzTabArrowEh.cpp - the /GX EH-framed CSBI_StatzTabArrow destructor
// (C:\Proj\Gruntz). The split off the frameless base TU is matching-neutral (each
// function is RVA-keyed). Chain: CSBI_StatzTabArrow : CSBI_ImageSetAni :
// CSBI_ImageSet : CSBI_Image : CSBI_RectOnly : CStatusBarItem (five bases from
// the shared SbiDtorChain.h).

SIZE_UNKNOWN(CSBI_StatzTabArrow);

RVA(0x001048f0, 0xa9)
CSBI_StatzTabArrow::~CSBI_StatzTabArrow() {
    DtorStatzTabArrow();
}
