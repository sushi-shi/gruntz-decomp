// SBI_StatzTabArrowEh.cpp - the /GX EH-framed CSBI_StatzTabArrow destructor
// (0x1048f0, C:\Proj\Gruntz). CSBI_StatzTabArrow is the deepest SBI leaf; its
// most-derived dtor folds the whole five-level subobject teardown walk
//   CSBI_StatzTabArrow : CSBI_ImageSetAni : CSBI_ImageSet : CSBI_Image :
//   CSBI_RectOnly : CStatusBarItem
// behind one /GX SEH frame with descending trylevels. Kept a standalone /GX TU (its
// method side lives in a different obj); the split off the frameless bases is
// matching-neutral (RVA-keyed).
//
// Ported onto the canonical CHAIN-DTOR device (SBI_DTOR_CHAIN + the SBI_OWN_*_DTOR
// guards in the SBI_*.h chain headers, see StatusBarItem.h). Was on the retired
// <Gruntz/SbiDtorChain.h>; byte-neutral (identical chain shapes).
#define SBI_DTOR_CHAIN // enable the inline base-dtor bodies down the chain
#include <rva.h>
#include <Ints.h>
#include <Gruntz/SBI_ImageSetAni.h> // canonical chain up to CSBI_ImageSetAni + CSBI_StatzTabArrow

RVA(0x001048f0, 0xa9)
CSBI_StatzTabArrow::~CSBI_StatzTabArrow() {
    DtorStatzTabArrow();
}
