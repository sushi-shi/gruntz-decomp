#include <rva.h>
#include <Ints.h>

#define SBI_OWN_IMAGE_DTOR // this TU supplies the out-of-line ~CSBI_Image
#include <Gruntz/SbiDtorChain.h>
// SBI_ImageEh.cpp - the /GX EH-framed CSBI_Image destructor (0x100870), split off
// the frameless sbi_image TU (C:\Proj\Gruntz). MSVC5's /GX frames the dtor's
// base-subobject teardown walk; it cannot share the base TU's frameless flags.
// The split is matching-neutral (each function is RVA-keyed).
//
// 3-level case of docs/patterns/eh-dtor-multilevel-polymorphic-chain.md. CSBI_Image
// is the OUT-OF-LINE leaf here (SBI_OWN_IMAGE_DTOR guard) but the INLINE base of
// ~CSBI_ImageSet / ~CSBI_MenuItem elsewhere.

// The most-derived destructor: stamp ??_7CSBI_Image, run DtorImage, then MSVC folds
// the base dtors (stamp ??_7CSBI_RectOnly + DtorRect, stamp ??_7CStatusBarItem +
// DtorStatus). The non-trivial base subobjects supply the /GX frame and the
// 0/1/-1 trylevel stamps.
RVA(0x00100870, 0x6a)
CSBI_Image::~CSBI_Image() {
    DtorImage();
}
