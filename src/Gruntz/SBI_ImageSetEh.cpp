#include <rva.h>
#include <Ints.h>

#define SBI_OWN_IMAGESET_DTOR // this TU supplies the out-of-line ~CSBI_ImageSet
#include <Gruntz/SbiDtorChain.h>
// SBI_ImageSetEh.cpp - the /GX EH-framed CSBI_ImageSet scalar destructor
// (0x102000), split off SBI_RectOnlyEh.cpp (C:\Proj\Gruntz). MSVC5's /GX frames
// the dtor's base-subobject teardown walk; the split is matching-neutral (each
// function is RVA-keyed).
//
// 4-level case of docs/patterns/eh-dtor-multilevel-polymorphic-chain.md. CSBI_ImageSet
// is the OUT-OF-LINE leaf here (SBI_OWN_IMAGESET_DTOR guard) but the INLINE base of
// ~CSBI_ImageSetAni / ~CSBI_WarlordHead elsewhere. It lives in its own TU (not
// SBI_RectOnlyDtorEh.cpp) because the shared base CSBI_RectOnly is the INLINE base
// here but the OUT-OF-LINE leaf there.

// Stamp ??_7CSBI_ImageSet, run DtorImageSet, then MSVC folds the three base dtors
// (CSBI_Image, CSBI_RectOnly, CStatusBarItem) in. The non-trivial base subobjects
// supply the /GX frame and the 0/1/2/-1 trylevel stamps.
RVA(0x00102000, 0x7f)
CSBI_ImageSet::~CSBI_ImageSet() {
    DtorImageSet();
}
