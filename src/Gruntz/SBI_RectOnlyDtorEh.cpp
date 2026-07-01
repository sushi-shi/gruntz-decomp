#include <rva.h>
#include <Ints.h>

#define SBI_OWN_RECTONLY_DTOR // this TU supplies the out-of-line ~CSBI_RectOnly
#include <Gruntz/SbiDtorChain.h>
// SBI_RectOnlyDtorEh.cpp - the /GX EH-framed CSBI_RectOnly scalar destructor
// (0x100700), split off SBI_RectOnlyEh.cpp (C:\Proj\Gruntz). MSVC5's /GX frames
// the dtor's base-subobject teardown walk; it cannot share the frameless flags.
// The split is matching-neutral (each function is RVA-keyed).
//
// 2-level case of docs/patterns/eh-dtor-multilevel-polymorphic-chain.md. It MUST
// live in its own TU because the shared base CSBI_RectOnly is the OUT-OF-LINE leaf
// here but the INLINE base of ~CSBI_ImageSet in SBI_ImageSetEh.cpp - one TU cannot
// define it both ways (hence the SBI_OWN_RECTONLY_DTOR guard above).

// Stamp ??_7CSBI_RectOnly, run DtorRect, then MSVC folds the base dtor (stamp
// ??_7CStatusBarItem + DtorStatus). The non-trivial base subobject supplies the
// /GX frame and the 0/-1 trylevel stamps.
RVA(0x00100700, 0x55)
CSBI_RectOnly::~CSBI_RectOnly() {
    DtorRect();
}
