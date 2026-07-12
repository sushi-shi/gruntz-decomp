// SBI_RectOnlyDtorEh.cpp - the /GX EH-framed CSBI_RectOnly scalar destructor
// (0x100700), split off SBI_RectOnlyEh.cpp (C:\Proj\Gruntz). MSVC5's /GX frames
// the dtor's base-subobject teardown walk; it cannot share the frameless flags.
// The split is matching-neutral (each function is RVA-keyed).
//
// 2-level case of docs/patterns/eh-dtor-multilevel-polymorphic-chain.md. It stays its
// own /GX TU because the shared base CSBI_RectOnly is the OUT-OF-LINE leaf here but the
// INLINE base of ~CSBI_ImageSet elsewhere - one TU cannot define it both ways (hence
// the SBI_OWN_RECTONLY_DTOR guard below).
//
// Ported onto the canonical CHAIN-DTOR device (SBI_DTOR_CHAIN + SBI_OWN_*_DTOR in the
// SBI_*.h chain headers, see StatusBarItem.h). Was on the retired
// <Gruntz/SbiDtorChain.h>; byte-neutral (identical chain shapes).
#define SBI_DTOR_CHAIN        // enable the inline base-dtor bodies down the chain
#define SBI_OWN_RECTONLY_DTOR // this TU supplies the out-of-line ~CSBI_RectOnly (0x100700)
#include <rva.h>
#include <Ints.h>
#include <Gruntz/SBI_Image.h> // canonical CSBI_RectOnly chain + CStatusBarItem device

// Stamp ??_7CSBI_RectOnly, run DtorRect, then MSVC folds the base dtor (stamp
// ??_7CStatusBarItem + DtorStatus). The non-trivial base subobject supplies the
// /GX frame and the 0/-1 trylevel stamps.
RVA(0x00100700, 0x55)
CSBI_RectOnly::~CSBI_RectOnly() {
    DtorRect();
}

// -------------------------------------------------------------------------
// 0x100780 == the standalone out-of-line CStatusBarItem::~CStatusBarItem() COMDAT.
// The /GX funclet above ODR-uses ??1CStatusBarItem (the base-subobject unwind leg),
// so cl emits its inline body (`mov [ecx],??_7CStatusBarItem; jmp DtorStatus`) as an
// out-of-line COMDAT - byte-identical to retail 0x100780. Bind that emitted symbol to
// its RVA (like the ??_G scalar-dtor above); this replaces the former fake
// CStatusBaseSub100780 placeholder view (whose ??_7/Base1d6b reloc-masked the real
// ??_7CStatusBarItem/DtorStatus by shape) and binds BOTH the funclet reloc @0x100700
// and this body's DtorStatus @0x10bfa0.
// @rva-symbol: ??1CStatusBarItem@@UAE@XZ 0x00100780 0xb
